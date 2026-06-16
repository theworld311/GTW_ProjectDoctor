#include "SProjectDoctorTab.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IPythonScriptPlugin.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "ProjectDoctorTab"

// Column IDs
static const FName Col_Severity(TEXT("Severity"));
static const FName Col_Rule(TEXT("Rule"));
static const FName Col_Asset(TEXT("Asset"));
static const FName Col_Message(TEXT("Message"));

// -------------------------------------------------------------------------
// Row widget
// -------------------------------------------------------------------------
class SFindingRow : public SMultiColumnTableRow<FProjectDoctorFindingPtr>
{
public:
	SLATE_BEGIN_ARGS(SFindingRow) {}
		SLATE_ARGUMENT(FProjectDoctorFindingPtr, Item)
		SLATE_ARGUMENT(TFunction<FSlateColor(const FString&)>, ColorGetter)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
	{
		Item = InArgs._Item;
		ColorGetter = InArgs._ColorGetter;
		SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), InOwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		FText CellText;
		if      (ColumnName == Col_Severity) CellText = FText::FromString(Item->Severity);
		else if (ColumnName == Col_Rule)     CellText = FText::FromString(Item->Rule);
		else if (ColumnName == Col_Asset)    CellText = FText::FromString(Item->AssetPath);
		else                                 CellText = FText::FromString(Item->Message);

		TSharedRef<STextBlock> TextBlock = SNew(STextBlock)
			.Text(CellText)
			.ColorAndOpacity(ColumnName == Col_Severity ? ColorGetter(Item->Severity) : FSlateColor::UseForeground())
			.Font(ColumnName == Col_Severity
				? FCoreStyle::GetDefaultFontStyle("Bold", 9)
				: FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ToolTipText(ColumnName == Col_Asset ? FText::FromString(Item->AssetPath) : FText::GetEmpty());

		return SNew(SBox)
			.Padding(FMargin(6.f, 3.f))
			[TextBlock];
	}

private:
	FProjectDoctorFindingPtr Item;
	TFunction<FSlateColor(const FString&)> ColorGetter;
};

// -------------------------------------------------------------------------
// SProjectDoctorTab
// -------------------------------------------------------------------------
void SProjectDoctorTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)

		// Toolbar
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildToolbar()
		]

		// Summary bar
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildSummaryBar()
		]

		// Filter bar
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 4.f)
		[
			BuildFilterBar()
		]

		// List
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(2.f)
			[
				SAssignNew(ListView, SListView<FProjectDoctorFindingPtr>)
				.ListItemsSource(&FilteredFindings)
				.OnGenerateRow(this, &SProjectDoctorTab::GenerateRow)
				.OnMouseButtonDoubleClick(this, &SProjectDoctorTab::OnFindingDoubleClicked)
				.SelectionMode(ESelectionMode::Single)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(Col_Severity).DefaultLabel(LOCTEXT("ColSev",  "Severity")).FixedWidth(70.f)
					+ SHeaderRow::Column(Col_Rule)    .DefaultLabel(LOCTEXT("ColRule", "Rule"))    .FixedWidth(180.f)
					+ SHeaderRow::Column(Col_Asset)   .DefaultLabel(LOCTEXT("ColAsset","Asset"))   .FillWidth(1.f)
					+ SHeaderRow::Column(Col_Message) .DefaultLabel(LOCTEXT("ColMsg",  "Message")) .FillWidth(1.5f)
				)
			]
		]

		// Status bar
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("StatusBar.Background"))
			.Padding(FMargin(8.f, 3.f))
			[
				SAssignNew(StatusText, STextBlock)
				.Text(this, &SProjectDoctorTab::GetStatusText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
		]
	];

	// Load last report if one exists
	ReloadReport();
}

// -------------------------------------------------------------------------
// UI builders
// -------------------------------------------------------------------------
TSharedRef<SWidget> SProjectDoctorTab::BuildToolbar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("EditorViewportToolBar.Background"))
		.Padding(FMargin(4.f, 2.f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ScanGame", "Scan /Game"))
				.ToolTipText(LOCTEXT("ScanGameTip", "Scan all assets under /Game"))
				.OnClicked(this, &SProjectDoctorTab::OnScanGameClicked)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
				.ForegroundColor(FLinearColor::White)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ScanCustom", "Scan Custom Path..."))
				.ToolTipText(LOCTEXT("ScanCustomTip", "Choose a sub-folder of /Game to scan"))
				.OnClicked(this, &SProjectDoctorTab::OnScanCustomClicked)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.ToolTipText(LOCTEXT("RefreshTip", "Reload the latest report from disk without rescanning"))
				.OnClicked(this, &SProjectDoctorTab::OnRefreshClicked)
			]

			// Spacer
			+ SHorizontalBox::Slot().FillWidth(1.f)

			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenHtml", "Open HTML Report"))
				.ToolTipText(LOCTEXT("OpenHtmlTip", "Open the full HTML report in your browser"))
				.OnClicked(this, &SProjectDoctorTab::OnOpenHtmlClicked)
			]
		];
}

TSharedRef<SWidget> SProjectDoctorTab::BuildSummaryBar()
{
	auto MakeCard = [](const FText& Label, TAttribute<FText> Value, FLinearColor ValueColor) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(10.f, 6.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(Value)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				.ColorAndOpacity(ValueColor)
			]
		];
	};

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[ MakeCard(LOCTEXT("CardScanned",  "Assets Scanned"),
				TAttribute<FText>::CreateLambda([this]{ return FText::AsNumber(ScannedAssets); }),
				FLinearColor::White) ]

			+ SHorizontalBox::Slot().AutoWidth()
			[ MakeCard(LOCTEXT("CardTotal", "Total Findings"),
				TAttribute<FText>::CreateLambda([this]{ return FText::AsNumber(AllFindings.Num()); }),
				FLinearColor::White) ]

			+ SHorizontalBox::Slot().AutoWidth()
			[ MakeCard(LOCTEXT("CardErrors", "Errors"),
				TAttribute<FText>::CreateLambda([this]{ return FText::AsNumber(TotalErrors); }),
				FLinearColor(1.f, 0.25f, 0.25f)) ]

			+ SHorizontalBox::Slot().AutoWidth()
			[ MakeCard(LOCTEXT("CardWarnings", "Warnings"),
				TAttribute<FText>::CreateLambda([this]{ return FText::AsNumber(TotalWarnings); }),
				FLinearColor(1.f, 0.65f, 0.15f)) ]

			+ SHorizontalBox::Slot().AutoWidth()
			[ MakeCard(LOCTEXT("CardInfo", "Info"),
				TAttribute<FText>::CreateLambda([this]{ return FText::AsNumber(TotalInfo); }),
				FLinearColor(0.35f, 0.7f, 1.f)) ]

			+ SHorizontalBox::Slot().FillWidth(1.f)

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(this, &SProjectDoctorTab::GetSummaryText)
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
				.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			]
		];
}

TSharedRef<SWidget> SProjectDoctorTab::BuildFilterBar()
{
	auto MakeFilterBtn = [this](const FText& Label, FName Severity, FLinearColor ActiveColor) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.Text(Label)
			.OnClicked_Lambda([this, Severity]() -> FReply
			{
				OnFilterChanged(Severity);
				return FReply::Handled();
			})
			.ButtonColorAndOpacity_Lambda([this, Severity, ActiveColor]() -> FLinearColor
			{
				return (ActiveFilter == Severity) ? ActiveColor : FLinearColor(0.2f, 0.2f, 0.2f);
			})
			.ForegroundColor(FLinearColor::White)
			.ContentPadding(FMargin(12.f, 3.f));
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
		[ MakeFilterBtn(LOCTEXT("FilterAll",     "All"),      NAME_None,             FLinearColor(0.3f, 0.3f, 0.3f)) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
		[ MakeFilterBtn(LOCTEXT("FilterErrors",  "Errors"),   FName("Error"),        FLinearColor(0.7f, 0.1f, 0.1f)) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
		[ MakeFilterBtn(LOCTEXT("FilterWarnings","Warnings"), FName("Warning"),      FLinearColor(0.75f, 0.4f, 0.0f)) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
		[ MakeFilterBtn(LOCTEXT("FilterInfo",   "Info"),      FName("Info"),         FLinearColor(0.1f, 0.4f, 0.75f)) ];
}

// -------------------------------------------------------------------------
// List row
// -------------------------------------------------------------------------
TSharedRef<ITableRow> SProjectDoctorTab::GenerateRow(
	FProjectDoctorFindingPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SFindingRow, OwnerTable)
		.Item(Item)
		.ColorGetter([this](const FString& Sev) { return GetSeverityColor(Sev); });
}

// -------------------------------------------------------------------------
// Actions
// -------------------------------------------------------------------------
FReply SProjectDoctorTab::OnScanGameClicked()
{
	RunScan(TEXT("/Game"), 5000);
	return FReply::Handled();
}

FReply SProjectDoctorTab::OnScanCustomClicked()
{
	// Simple input dialog via a small modal
	TSharedRef<SEditableTextBox> InputBox = SNew(SEditableTextBox)
		.Text(LOCTEXT("DefaultPath", "/Game/"))
		.MinDesiredWidth(300.f);

	TSharedRef<SWindow> ModalWindow = SNew(SWindow)
		.Title(LOCTEXT("CustomScanTitle", "Scan Custom Path"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SBox).Padding(16.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CustomScanLabel", "Enter a /Game sub-path to scan:"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
				[ InputBox ]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("OK", "Scan"))
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
						.ForegroundColor(FLinearColor::White)
						.OnClicked_Lambda([this, &InputBox, &ModalWindow]() -> FReply
						{
							FString Path = InputBox->GetText().ToString();
							ModalWindow->RequestDestroyWindow();
							RunScan(Path, 5000);
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("Cancel", "Cancel"))
						.OnClicked_Lambda([&ModalWindow]() -> FReply
						{
							ModalWindow->RequestDestroyWindow();
							return FReply::Handled();
						})
					]
				]
			]
		];

	GEditor->EditorAddModalWindow(ModalWindow);
	return FReply::Handled();
}

FReply SProjectDoctorTab::OnOpenHtmlClicked()
{
	if (!LastHtmlPath.IsEmpty() && FPaths::FileExists(LastHtmlPath))
	{
		FPlatformProcess::LaunchURL(*FString::Printf(TEXT("file:///%s"), *LastHtmlPath), nullptr, nullptr);
	}
	return FReply::Handled();
}

FReply SProjectDoctorTab::OnRefreshClicked()
{
	ReloadReport();
	return FReply::Handled();
}

void SProjectDoctorTab::OnFilterChanged(FName Severity)
{
	ActiveFilter = Severity;
	ApplyFilter();
}

void SProjectDoctorTab::OnFindingDoubleClicked(FProjectDoctorFindingPtr Item)
{
	if (!Item.IsValid()) return;

	// Strip object path suffix (e.g. "/Game/Meshes/SM_Rock.SM_Rock" -> "/Game/Meshes/SM_Rock")
	FString PackagePath = Item->AssetPath;
	int32 DotIdx;
	if (PackagePath.FindLastChar(TEXT('.'), DotIdx))
	{
		PackagePath = PackagePath.Left(DotIdx);
	}

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	AssetRegistry.Get().GetAssetsByPackageName(*PackagePath, Assets);

	if (Assets.Num() > 0)
	{
		FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowser.Get().SyncBrowserToAssets(Assets);
	}
}

// -------------------------------------------------------------------------
// Scan via Python
// -------------------------------------------------------------------------
void SProjectDoctorTab::RunScan(const FString& RootPath, int32 MaxAssets)
{
	if (!IPythonScriptPlugin::Get()->IsPythonAvailable())
	{
		UE_LOG(LogTemp, Warning, TEXT("ProjectDoctor: Python is not available. Enable the Python Script Plugin."));
		return;
	}

	bScanning = true;

	const FString PluginPythonDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("ProjectDoctor/Content/Python")));

	FString Command = FString::Printf(
		TEXT("import sys, os; p=r'%s'; (sys.path.append(p) if p not in sys.path else None); "
		     "import importlib, project_doctor; importlib.reload(project_doctor); "
		     "project_doctor.scan_project(root_path='%s', max_assets=%d, export=True)"),
		*PluginPythonDir, *RootPath, MaxAssets);

	IPythonScriptPlugin::Get()->ExecPythonCommand(*Command);

	bScanning = false;
	LastRootPath = RootPath;
	ReloadReport();
}

// -------------------------------------------------------------------------
// Load JSON report
// -------------------------------------------------------------------------
void SProjectDoctorTab::ReloadReport()
{
	AllFindings.Empty();
	TotalErrors = TotalWarnings = TotalInfo = ScannedAssets = 0;
	LastHtmlPath.Empty();

	const FString JsonPath = GetLatestJsonPath();
	if (!FPaths::FileExists(JsonPath))
	{
		ApplyFilter();
		return;
	}

	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath)) return;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

	// Summary
	const TSharedPtr<FJsonObject>* SummaryObj;
	if (Root->TryGetObjectField(TEXT("summary"), SummaryObj))
	{
		(*SummaryObj)->TryGetNumberField(TEXT("scanned_assets"), ScannedAssets);
		(*SummaryObj)->TryGetNumberField(TEXT("errors"),         TotalErrors);
		(*SummaryObj)->TryGetNumberField(TEXT("warnings"),       TotalWarnings);
		(*SummaryObj)->TryGetNumberField(TEXT("info"),           TotalInfo);
		(*SummaryObj)->TryGetStringField(TEXT("root_path"),      LastRootPath);
	}

	// Findings
	const TArray<TSharedPtr<FJsonValue>>* FindingsArr;
	if (Root->TryGetArrayField(TEXT("findings"), FindingsArr))
	{
		for (const TSharedPtr<FJsonValue>& Val : *FindingsArr)
		{
			const TSharedPtr<FJsonObject>* Obj;
			if (!Val->TryGetObject(Obj)) continue;

			FProjectDoctorFindingPtr Finding = MakeShared<FProjectDoctorFinding>();
			(*Obj)->TryGetStringField(TEXT("severity"),   Finding->Severity);
			(*Obj)->TryGetStringField(TEXT("rule"),       Finding->Rule);
			(*Obj)->TryGetStringField(TEXT("asset_path"), Finding->AssetPath);
			(*Obj)->TryGetStringField(TEXT("message"),    Finding->Message);
			AllFindings.Add(Finding);
		}
	}

	// Derive HTML path from JSON path
	LastHtmlPath = JsonPath.Replace(TEXT("latest_project_doctor_report.json"), TEXT("")) ;
	// Try to find the most recent HTML
	TArray<FString> HtmlFiles;
	IFileManager::Get().FindFiles(HtmlFiles,
		*(FPaths::GetPath(JsonPath) / TEXT("*.html")), true, false);
	if (HtmlFiles.Num() > 0)
	{
		HtmlFiles.Sort();
		LastHtmlPath = FPaths::GetPath(JsonPath) / HtmlFiles.Last();
	}

	ApplyFilter();
}

void SProjectDoctorTab::ApplyFilter()
{
	FilteredFindings.Empty();
	for (const FProjectDoctorFindingPtr& F : AllFindings)
	{
		if (ActiveFilter == NAME_None || F->Severity == ActiveFilter.ToString())
		{
			FilteredFindings.Add(F);
		}
	}
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------
FString SProjectDoctorTab::GetLatestJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(),
		TEXT("ProjectDoctorReports"), TEXT("latest_project_doctor_report.json"));
}

FSlateColor SProjectDoctorTab::GetSeverityColor(const FString& Severity) const
{
	if (Severity == TEXT("Error"))   return FLinearColor(1.f, 0.3f, 0.3f);
	if (Severity == TEXT("Warning")) return FLinearColor(1.f, 0.7f, 0.2f);
	return FLinearColor(0.5f, 0.8f, 1.f);
}

FText SProjectDoctorTab::GetStatusText() const
{
	if (bScanning) return LOCTEXT("Scanning", "Scanning...");
	if (AllFindings.Num() == 0) return LOCTEXT("NoReport", "No report loaded. Click 'Scan /Game' to get started.");
	return FText::Format(
		LOCTEXT("Status", "Showing {0} of {1} findings  |  Root: {2}"),
		FText::AsNumber(FilteredFindings.Num()),
		FText::AsNumber(AllFindings.Num()),
		FText::FromString(LastRootPath));
}

FText SProjectDoctorTab::GetSummaryText() const
{
	if (AllFindings.Num() == 0) return FText::GetEmpty();
	return FText::Format(LOCTEXT("SummaryHint", "Double-click a finding to locate it in the Content Browser."),
		FText::AsNumber(AllFindings.Num()));
}

#undef LOCTEXT_NAMESPACE
