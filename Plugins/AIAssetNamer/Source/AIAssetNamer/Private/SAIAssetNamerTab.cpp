#include "SAIAssetNamerTab.h"
#include "AIAssetNamerSettings.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Misc/MessageDialog.h"
#include "IPythonScriptPlugin.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "SAIAssetNamerTab"

// -------------------------------------------------------------------------
// UE5 naming prefix map
// -------------------------------------------------------------------------
static TMap<FString, FString> GetPrefixMap()
{
	TMap<FString, FString> M;
	M.Add(TEXT("StaticMesh"),         TEXT("SM_"));
	M.Add(TEXT("SkeletalMesh"),       TEXT("SK_"));
	M.Add(TEXT("Texture2D"),          TEXT("T_"));
	M.Add(TEXT("TextureCube"),        TEXT("T_"));
	M.Add(TEXT("TextureRenderTarget2D"), TEXT("RT_"));
	M.Add(TEXT("Material"),           TEXT("M_"));
	M.Add(TEXT("MaterialInstance"),   TEXT("MI_"));
	M.Add(TEXT("MaterialInstanceConstant"), TEXT("MI_"));
	M.Add(TEXT("Blueprint"),          TEXT("BP_"));
	M.Add(TEXT("AnimBlueprint"),      TEXT("ABP_"));
	M.Add(TEXT("WidgetBlueprint"),    TEXT("WBP_"));
	M.Add(TEXT("SoundWave"),          TEXT("A_"));
	M.Add(TEXT("SoundCue"),           TEXT("AS_"));
	M.Add(TEXT("AnimSequence"),       TEXT("AS_"));
	M.Add(TEXT("AnimMontage"),        TEXT("AM_"));
	M.Add(TEXT("BlendSpace"),         TEXT("BS_"));
	M.Add(TEXT("PhysicsAsset"),       TEXT("PHYS_"));
	M.Add(TEXT("DataTable"),          TEXT("DT_"));
	M.Add(TEXT("DataAsset"),          TEXT("DA_"));
	M.Add(TEXT("UserDefinedEnum"),    TEXT("E_"));
	M.Add(TEXT("UserDefinedStruct"),  TEXT("F_"));
	M.Add(TEXT("NiagaraSystem"),      TEXT("NS_"));
	M.Add(TEXT("NiagaraEmitter"),     TEXT("NE_"));
	return M;
}

// -------------------------------------------------------------------------
// Construct
// -------------------------------------------------------------------------
void SAIAssetNamerTab::Construct(const FArguments& InArgs)
{
	ProviderOptions.Add(MakeShareable(new FString(TEXT("Claude (Anthropic)"))));
	ProviderOptions.Add(MakeShareable(new FString(TEXT("OpenAI (GPT-4o)"))));
	SelectedProvider = ProviderOptions[0];

	TSharedPtr<SVerticalBox> VBox = SNew(SVerticalBox);

	// Toolbar
	VBox->AddSlot().AutoHeight().Padding(4, 4, 4, 2)
	[
		BuildToolbar()
	];

	// Settings bar (API key + provider)
	VBox->AddSlot().AutoHeight().Padding(4, 2, 4, 2)
	[
		BuildSettingsBar()
	];

	// Summary bar
	VBox->AddSlot().AutoHeight().Padding(4, 2, 4, 4)
	[
		BuildSummaryBar()
	];

	// Separator
	VBox->AddSlot().AutoHeight()
	[
		SNew(SSeparator)
	];

	// List header
	VBox->AddSlot().AutoHeight().Padding(4, 4, 4, 0)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.05f)
		[
			SNew(STextBlock).Text(LOCTEXT("ColSel", "")).MinDesiredWidth(20)
		]
		+ SHorizontalBox::Slot().FillWidth(0.25f)
		[
			SNew(STextBlock).Text(LOCTEXT("ColCurrent", "Current Name"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot().FillWidth(0.25f)
		[
			SNew(STextBlock).Text(LOCTEXT("ColSuggested", "Suggested Name"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot().FillWidth(0.30f)
		[
			SNew(STextBlock).Text(LOCTEXT("ColAsset", "Asset Path"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot().FillWidth(0.15f)
		[
			SNew(STextBlock).Text(LOCTEXT("ColStatus", "Status"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
	];

	VBox->AddSlot().AutoHeight()
	[
		SNew(SSeparator)
	];

	// List view
	VBox->AddSlot().FillHeight(1.0f).Padding(4, 0, 4, 4)
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SAssignNew(ListView, SListView<FAIAssetSuggestionPtr>)
			.ListItemsSource(&Suggestions)
			.OnGenerateRow(this, &SAIAssetNamerTab::GenerateRow)
			.SelectionMode(ESelectionMode::Single)
		]
	];

	// Status bar
	VBox->AddSlot().AutoHeight().Padding(4, 2)
	[
		SAssignNew(StatusTextBlock, STextBlock)
		.Text(LOCTEXT("StatusReady", "Ready. Load assets and click Suggest Names."))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
	];

	ChildSlot
	[
		VBox.ToSharedRef()
	];
}

// -------------------------------------------------------------------------
// BuildToolbar
// -------------------------------------------------------------------------
TSharedRef<SWidget> SAIAssetNamerTab::BuildToolbar()
{
	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("BtnLoadSelected", "Load Selected Assets"))
		.ToolTipText(LOCTEXT("BtnLoadSelectedTT", "Load assets currently selected in the Content Browser"))
		.OnClicked(this, &SAIAssetNamerTab::OnLoadSelectedClicked)
	]

	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("BtnLoadFolder", "Load Folder..."))
		.ToolTipText(LOCTEXT("BtnLoadFolderTT", "Load all assets from a specific folder path"))
		.OnClicked(this, &SAIAssetNamerTab::OnLoadFolderClicked)
	]

	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("BtnSuggest", "Suggest Names (AI)"))
		.ToolTipText(LOCTEXT("BtnSuggestTT", "Send loaded assets to AI and get name suggestions"))
		.OnClicked(this, &SAIAssetNamerTab::OnSuggestNamesClicked)
	]

	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("BtnApply", "Apply Selected"))
		.ToolTipText(LOCTEXT("BtnApplyTT", "Rename all checked assets using the suggested names"))
		.OnClicked(this, &SAIAssetNamerTab::OnApplySelectedClicked)
	]

	+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("BtnClear", "Clear"))
		.OnClicked(this, &SAIAssetNamerTab::OnClearClicked)
	];
}

// -------------------------------------------------------------------------
// BuildSettingsBar
// -------------------------------------------------------------------------
TSharedRef<SWidget> SAIAssetNamerTab::BuildSettingsBar()
{
	UAIAssetNamerSettings* Settings = UAIAssetNamerSettings::Get();

	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
	[
		SNew(STextBlock).Text(LOCTEXT("LabelProvider", "Provider:"))
	]

	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
	[
		SAssignNew(ProviderCombo, SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&ProviderOptions)
		.InitiallySelectedItem(SelectedProvider)
		.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
		{
			SelectedProvider = Item;
		})
		.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
		{
			return SNew(STextBlock).Text(FText::FromString(*Item));
		})
		[
			SNew(STextBlock)
			.Text_Lambda([this]() -> FText
			{
				return SelectedProvider.IsValid() ? FText::FromString(*SelectedProvider) : FText::GetEmpty();
			})
		]
	]

	+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
	[
		SNew(STextBlock).Text(LOCTEXT("LabelApiKey", "API Key:"))
	]

	+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 6, 0)
	[
		SAssignNew(ApiKeyBox, SEditableTextBox)
		.Text(FText::FromString(Settings->ApiKey))
		.IsPassword(true)
		.HintText(LOCTEXT("ApiKeyHint", "Paste your API key here..."))
		.OnTextCommitted_Lambda([](const FText& Text, ETextCommit::Type)
		{
			UAIAssetNamerSettings* S = UAIAssetNamerSettings::Get();
			S->ApiKey = Text.ToString();
			S->SaveConfig();
		})
	]

	+ SHorizontalBox::Slot().AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("BtnSaveKey", "Save"))
		.OnClicked_Lambda([this]() -> FReply
		{
			UAIAssetNamerSettings* S = UAIAssetNamerSettings::Get();
			S->ApiKey = ApiKeyBox->GetText().ToString();
			S->SaveConfig();
			StatusTextBlock->SetText(LOCTEXT("KeySaved", "API key saved."));
			return FReply::Handled();
		})
	];
}

// -------------------------------------------------------------------------
// BuildSummaryBar
// -------------------------------------------------------------------------
TSharedRef<SWidget> SAIAssetNamerTab::BuildSummaryBar()
{
	auto MakeCard = [](const FText& Label, TAttribute<FText> Value, FLinearColor Color) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.Padding(FMargin(10, 6))
			.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(Value)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.ColorAndOpacity(FSlateColor(Color))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(Label)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
				]
			];
	};

	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth().Padding(2)
	[
		MakeCard(
			LOCTEXT("CardLoaded", "Loaded"),
			TAttribute<FText>::CreateLambda([this]() { return FText::AsNumber(Suggestions.Num()); }),
			FLinearColor(0.6f, 0.8f, 1.0f)
		)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2)
	[
		MakeCard(
			LOCTEXT("CardSuggested", "Suggested"),
			TAttribute<FText>::CreateLambda([this]()
			{
				int32 N = 0;
				for (auto& S : Suggestions) if (S->Status == TEXT("Suggested") || S->Status == TEXT("Applied")) N++;
				return FText::AsNumber(N);
			}),
			FLinearColor(0.4f, 1.0f, 0.6f)
		)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2)
	[
		MakeCard(
			LOCTEXT("CardApplied", "Applied"),
			TAttribute<FText>::CreateLambda([this]()
			{
				int32 N = 0;
				for (auto& S : Suggestions) if (S->Status == TEXT("Applied")) N++;
				return FText::AsNumber(N);
			}),
			FLinearColor(1.0f, 0.85f, 0.3f)
		)
	];
}

// -------------------------------------------------------------------------
// GenerateRow
// -------------------------------------------------------------------------
TSharedRef<ITableRow> SAIAssetNamerTab::GenerateRow(FAIAssetSuggestionPtr Item, const TSharedRef<STableViewBase>& Owner)
{
	FLinearColor StatusColor = GetStatusColor(Item->Status).GetSpecifiedColor();

	return SNew(STableRow<FAIAssetSuggestionPtr>, Owner)
	.Padding(FMargin(2))
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(0.05f).VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(Item->bSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged(this, &SAIAssetNamerTab::OnRowCheckChanged, Item)
		]

		+ SHorizontalBox::Slot().FillWidth(0.25f).VAlign(VAlign_Center).Padding(2, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Item->CurrentName))
			.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.5f, 0.4f)))
		]

		+ SHorizontalBox::Slot().FillWidth(0.25f).VAlign(VAlign_Center).Padding(2, 0)
		[
			SNew(STextBlock)
			.Text_Lambda([Item]()
			{
				return Item->SuggestedName.IsEmpty()
					? LOCTEXT("Pending", "---")
					: FText::FromString(Item->SuggestedName);
			})
			.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 1.0f, 0.6f)))
		]

		+ SHorizontalBox::Slot().FillWidth(0.30f).VAlign(VAlign_Center).Padding(2, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Item->AssetPath))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))
		]

		+ SHorizontalBox::Slot().FillWidth(0.15f).VAlign(VAlign_Center).Padding(2, 0)
		[
			SNew(STextBlock)
			.Text_Lambda([Item]() { return FText::FromString(Item->Status); })
			.ColorAndOpacity_Lambda([this, Item]() { return GetStatusColor(Item->Status); })
		]
	];
}

// -------------------------------------------------------------------------
// Load Selected Assets from Content Browser
// -------------------------------------------------------------------------
FReply SAIAssetNamerTab::OnLoadSelectedClicked()
{
	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> Selected;
	CB.Get().GetSelectedAssets(Selected);

	if (Selected.Num() == 0)
	{
		StatusTextBlock->SetText(LOCTEXT("NoSelection", "No assets selected in Content Browser."));
		return FReply::Handled();
	}

	Suggestions.Empty();
	for (const FAssetData& AD : Selected)
	{
		FAIAssetSuggestionPtr S = MakeShareable(new FAIAssetSuggestion());
		S->AssetPath    = AD.GetObjectPathString();
		S->AssetClass   = AD.AssetClassPath.GetAssetName().ToString();
		S->CurrentName  = AD.AssetName.ToString();
		S->SuggestedName = TEXT("");
		S->Status       = TEXT("Pending");
		S->bSelected    = true;
		Suggestions.Add(S);
	}

	ListView->RequestListRefresh();
	StatusTextBlock->SetText(FText::Format(
		LOCTEXT("LoadedFmt", "Loaded {0} asset(s). Click 'Suggest Names (AI)' to continue."),
		FText::AsNumber(Suggestions.Num())
	));
	return FReply::Handled();
}

// -------------------------------------------------------------------------
// Load Folder
// -------------------------------------------------------------------------
FReply SAIAssetNamerTab::OnLoadFolderClicked()
{
	FString FolderPath;
	// Use a simple dialog to ask for folder path
	TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("FolderDialogTitle", "Enter Folder Path"))
		.ClientSize(FVector2D(420, 80))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	TSharedPtr<SEditableTextBox> PathBox;

	DialogWindow->SetContent(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8)
		[
			SAssignNew(PathBox, SEditableTextBox)
			.HintText(LOCTEXT("FolderHint", "/Game/Characters"))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4)
		[
			SNew(SButton)
			.Text(LOCTEXT("OK", "OK"))
			.OnClicked_Lambda([&DialogWindow, &PathBox, &FolderPath]() -> FReply
			{
				FolderPath = PathBox->GetText().ToString();
				DialogWindow->RequestDestroyWindow();
				return FReply::Handled();
			})
		]
	);

	GEditor->EditorAddModalWindow(DialogWindow);

	if (FolderPath.IsEmpty()) return FReply::Handled();

	FAssetRegistryModule& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	AR.Get().GetAssetsByPath(FName(*FolderPath), Assets, true);

	if (Assets.Num() == 0)
	{
		StatusTextBlock->SetText(LOCTEXT("NoAssets", "No assets found in that folder."));
		return FReply::Handled();
	}

	Suggestions.Empty();
	for (const FAssetData& AD : Assets)
	{
		FAIAssetSuggestionPtr S = MakeShareable(new FAIAssetSuggestion());
		S->AssetPath    = AD.GetObjectPathString();
		S->AssetClass   = AD.AssetClassPath.GetAssetName().ToString();
		S->CurrentName  = AD.AssetName.ToString();
		S->SuggestedName = TEXT("");
		S->Status       = TEXT("Pending");
		S->bSelected    = true;
		Suggestions.Add(S);
	}

	ListView->RequestListRefresh();
	StatusTextBlock->SetText(FText::Format(
		LOCTEXT("LoadedFolderFmt", "Loaded {0} asset(s) from {1}."),
		FText::AsNumber(Suggestions.Num()),
		FText::FromString(FolderPath)
	));
	return FReply::Handled();
}

// -------------------------------------------------------------------------
// Suggest Names — batch AI request
// -------------------------------------------------------------------------
FReply SAIAssetNamerTab::OnSuggestNamesClicked()
{
	if (Suggestions.Num() == 0)
	{
		StatusTextBlock->SetText(LOCTEXT("NoAssetsLoaded", "Load assets first."));
		return FReply::Handled();
	}

	UAIAssetNamerSettings* Settings = UAIAssetNamerSettings::Get();

	// Save whatever is in the key box right now
	Settings->ApiKey = ApiKeyBox->GetText().ToString();
	Settings->SaveConfig();

	if (Settings->ApiKey.IsEmpty())
	{
		StatusTextBlock->SetText(LOCTEXT("NoKey", "Please enter your API key first."));
		return FReply::Handled();
	}

	bRequesting = true;
	StatusTextBlock->SetText(LOCTEXT("Requesting", "Sending request to AI..."));

	SendBatchToAI();
	return FReply::Handled();
}

// -------------------------------------------------------------------------
// Build prompt for the AI
// -------------------------------------------------------------------------
FString SAIAssetNamerTab::BuildPrompt() const
{
	FString AssetList;
	int32 Idx = 0;
	for (const FAIAssetSuggestionPtr& S : Suggestions)
	{
		AssetList += FString::Printf(TEXT("%d. name=\"%s\" type=\"%s\"\n"), Idx++, *S->CurrentName, *S->AssetClass);
	}

	return FString::Printf(TEXT(
		"You are an Unreal Engine 5 asset naming expert.\n"
		"The standard naming prefixes are:\n"
		"StaticMesh=SM_, SkeletalMesh=SK_, Texture2D=T_, TextureCube=T_, TextureRenderTarget2D=RT_, "
		"Material=M_, MaterialInstance/MaterialInstanceConstant=MI_, Blueprint=BP_, AnimBlueprint=ABP_, "
		"WidgetBlueprint=WBP_, SoundWave=A_, SoundCue=AS_, AnimSequence=AS_, AnimMontage=AM_, "
		"BlendSpace=BS_, PhysicsAsset=PHYS_, DataTable=DT_, DataAsset=DA_, "
		"UserDefinedEnum=E_, UserDefinedStruct=F_, NiagaraSystem=NS_, NiagaraEmitter=NE_.\n\n"
		"For each asset below, suggest a corrected name following these rules:\n"
		"1. Add the correct prefix if missing\n"
		"2. Use PascalCase after the prefix\n"
		"3. Remove underscores from the descriptive part (except the prefix separator)\n"
		"4. Keep the name descriptive and short\n"
		"5. If the name is already correct, return it unchanged\n\n"
		"Assets:\n%s\n"
		"Respond ONLY with a JSON array in this exact format, no explanation:\n"
		"[{\"index\":0,\"suggested\":\"SM_Rock\"},{\"index\":1,\"suggested\":\"T_RockDiffuse\"},...]\n"
	), *AssetList);
}

// -------------------------------------------------------------------------
// Send HTTP request
// -------------------------------------------------------------------------
void SAIAssetNamerTab::SendBatchToAI()
{
	UAIAssetNamerSettings* Settings = UAIAssetNamerSettings::Get();
	const bool bUseClaude = !SelectedProvider.IsValid() || (*SelectedProvider).Contains(TEXT("Claude"));

	FString Url;
	FString AuthHeader;
	FString RequestBody;

	const FString Prompt = BuildPrompt();

	if (bUseClaude)
	{
		Url = TEXT("https://api.anthropic.com/v1/messages");
		AuthHeader = FString::Printf(TEXT("x-api-key: %s"), *Settings->ApiKey);

		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject());
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), Prompt);

		TArray<TSharedPtr<FJsonValue>> Messages;
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));

		TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject());
		Root->SetStringField(TEXT("model"), Settings->ClaudeModel);
		Root->SetNumberField(TEXT("max_tokens"), 4096);
		Root->SetArrayField(TEXT("messages"), Messages);

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
		RequestBody = OutputString;
	}
	else
	{
		Url = TEXT("https://api.openai.com/v1/chat/completions");
		AuthHeader = FString::Printf(TEXT("Bearer %s"), *Settings->ApiKey);

		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject());
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), Prompt);

		TArray<TSharedPtr<FJsonValue>> Messages;
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));

		TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject());
		Root->SetStringField(TEXT("model"), Settings->OpenAIModel);
		Root->SetNumberField(TEXT("max_tokens"), 4096);
		Root->SetArrayField(TEXT("messages"), Messages);

		FString OutputString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
		RequestBody = OutputString;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	if (bUseClaude)
	{
		Request->SetHeader(TEXT("x-api-key"), Settings->ApiKey);
		Request->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
	}
	else
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Settings->ApiKey));
	}

	Request->SetContentAsString(RequestBody);
	Request->OnProcessRequestComplete().BindRaw(this, &SAIAssetNamerTab::OnAIResponseReceived);
	Request->ProcessRequest();
}

// -------------------------------------------------------------------------
// HTTP response handler
// -------------------------------------------------------------------------
void SAIAssetNamerTab::OnAIResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	bRequesting = false;

	if (!bSuccess || !Response.IsValid())
	{
		StatusTextBlock->SetText(LOCTEXT("HttpFail", "Network error. Check your internet connection."));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		StatusTextBlock->SetText(FText::Format(
			LOCTEXT("HttpError", "API error {0}. Check your API key."),
			FText::AsNumber(Response->GetResponseCode())
		));
		return;
	}

	ParseAIResponse(Response->GetContentAsString());
}

// -------------------------------------------------------------------------
// Parse AI JSON response
// -------------------------------------------------------------------------
void SAIAssetNamerTab::ParseAIResponse(const FString& JsonBody)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonBody);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		StatusTextBlock->SetText(LOCTEXT("ParseFail", "Failed to parse API response."));
		return;
	}

	// Extract text content — works for both Claude and OpenAI response shapes
	FString RawText;

	// Claude shape: root.content[0].text
	const TArray<TSharedPtr<FJsonValue>>* ContentArr = nullptr;
	if (Root->TryGetArrayField(TEXT("content"), ContentArr) && ContentArr && ContentArr->Num() > 0)
	{
		const TSharedPtr<FJsonObject>* FirstBlock = nullptr;
		if ((*ContentArr)[0]->TryGetObject(FirstBlock) && FirstBlock)
		{
			(*FirstBlock)->TryGetStringField(TEXT("text"), RawText);
		}
	}

	// OpenAI shape: root.choices[0].message.content
	if (RawText.IsEmpty())
	{
		const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
		if (Root->TryGetArrayField(TEXT("choices"), Choices) && Choices && Choices->Num() > 0)
		{
			const TSharedPtr<FJsonObject>* Choice = nullptr;
			if ((*Choices)[0]->TryGetObject(Choice) && Choice)
			{
				const TSharedPtr<FJsonObject>* MsgObj = nullptr;
				if ((*Choice)->TryGetObjectField(TEXT("message"), MsgObj) && MsgObj)
				{
					(*MsgObj)->TryGetStringField(TEXT("content"), RawText);
				}
			}
		}
	}

	if (RawText.IsEmpty())
	{
		StatusTextBlock->SetText(LOCTEXT("EmptyResponse", "AI returned an empty response."));
		return;
	}

	// Find the JSON array in the text (AI sometimes adds extra text around it)
	int32 ArrayStart = RawText.Find(TEXT("["));
	int32 ArrayEnd   = RawText.Find(TEXT("]"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (ArrayStart == INDEX_NONE || ArrayEnd == INDEX_NONE)
	{
		StatusTextBlock->SetText(LOCTEXT("NoArray", "Could not find suggestions array in AI response."));
		return;
	}

	FString ArrayStr = RawText.Mid(ArrayStart, ArrayEnd - ArrayStart + 1);
	TArray<TSharedPtr<FJsonValue>> SuggestionsArray;
	TSharedRef<TJsonReader<>> ArrReader = TJsonReaderFactory<>::Create(ArrayStr);
	if (!FJsonSerializer::Deserialize(ArrReader, SuggestionsArray))
	{
		StatusTextBlock->SetText(LOCTEXT("ParseArrayFail", "Failed to parse suggestions from AI response."));
		return;
	}

	int32 Updated = 0;
	for (const TSharedPtr<FJsonValue>& Val : SuggestionsArray)
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (!Val->TryGetObject(ObjPtr) || !ObjPtr) continue;

		double IdxDouble = 0;
		FString Suggested;
		(*ObjPtr)->TryGetNumberField(TEXT("index"), IdxDouble);
		(*ObjPtr)->TryGetStringField(TEXT("suggested"), Suggested);

		int32 Idx = (int32)IdxDouble;
		if (Suggestions.IsValidIndex(Idx) && !Suggested.IsEmpty())
		{
			Suggestions[Idx]->SuggestedName = Suggested;
			Suggestions[Idx]->Status = TEXT("Suggested");
			Updated++;
		}
	}

	ListView->RequestListRefresh();
	StatusTextBlock->SetText(FText::Format(
		LOCTEXT("SuggestDone", "AI suggested names for {0} asset(s). Review and click 'Apply Selected'."),
		FText::AsNumber(Updated)
	));
}

// -------------------------------------------------------------------------
// Apply selected renames via Python
// -------------------------------------------------------------------------
FReply SAIAssetNamerTab::OnApplySelectedClicked()
{
	TArray<FAIAssetSuggestionPtr> ToApply;
	for (const FAIAssetSuggestionPtr& S : Suggestions)
	{
		if (S->bSelected && S->Status == TEXT("Suggested") && !S->SuggestedName.IsEmpty())
		{
			ToApply.Add(S);
		}
	}

	if (ToApply.Num() == 0)
	{
		StatusTextBlock->SetText(LOCTEXT("NothingToApply", "No suggested renames selected."));
		return FReply::Handled();
	}

	// Build Python rename script
	FString PythonScript = TEXT("import unreal\n");
	PythonScript += TEXT("asset_tools = unreal.AssetToolsHelpers.get_asset_tools()\n");
	PythonScript += TEXT("renames = []\n");

	for (const FAIAssetSuggestionPtr& S : ToApply)
	{
		FString AssetPath = S->AssetPath;
		int32 DotIdx;
		if (AssetPath.FindLastChar(TEXT('.'), DotIdx))
		{
			AssetPath = AssetPath.Left(DotIdx);
		}

		FString FolderPath = FPaths::GetPath(AssetPath);

		PythonScript += FString::Printf(
			TEXT("_asset = unreal.load_asset('%s')\n"),
			*AssetPath
		);
		PythonScript += TEXT("if _asset:\n");
		PythonScript += FString::Printf(
			TEXT("    _rd = unreal.AssetRenameData()\n")
		);
		PythonScript += TEXT("    _rd.asset = _asset\n");
		PythonScript += FString::Printf(TEXT("    _rd.new_name = '%s'\n"), *S->SuggestedName);
		PythonScript += FString::Printf(TEXT("    _rd.new_package_path = '%s'\n"), *FolderPath);
		PythonScript += TEXT("    renames.append(_rd)\n");
		PythonScript += FString::Printf(TEXT("    print('[AIAssetNamer] Queued: %s -> %s')\n"), *S->CurrentName, *S->SuggestedName);
		PythonScript += TEXT("else:\n");
		PythonScript += FString::Printf(TEXT("    print('[AIAssetNamer] Could not load: %s')\n"), *AssetPath);
	}

	PythonScript += TEXT("if renames:\n");
	PythonScript += TEXT("    asset_tools.rename_assets(renames)\n");
	PythonScript += TEXT("    print('[AIAssetNamer] Rename complete. Count: ' + str(len(renames)))\n");
	PythonScript += TEXT("else:\n");
	PythonScript += TEXT("    print('[AIAssetNamer] No assets to rename.')\n");

	if (IPythonScriptPlugin* Python = IPythonScriptPlugin::Get())
	{
		Python->ExecPythonCommand(*PythonScript);
	}

	for (const FAIAssetSuggestionPtr& S : ToApply)
	{
		S->Status = TEXT("Applied");
	}

	ListView->RequestListRefresh();
	StatusTextBlock->SetText(FText::Format(
		LOCTEXT("ApplyDone", "Renamed {0} asset(s) successfully."),
		FText::AsNumber(ToApply.Num())
	));
	return FReply::Handled();
}

// -------------------------------------------------------------------------
// Clear
// -------------------------------------------------------------------------
FReply SAIAssetNamerTab::OnClearClicked()
{
	Suggestions.Empty();
	ListView->RequestListRefresh();
	StatusTextBlock->SetText(LOCTEXT("Cleared", "Ready."));
	return FReply::Handled();
}

void SAIAssetNamerTab::OnRowCheckChanged(ECheckBoxState NewState, FAIAssetSuggestionPtr Item)
{
	Item->bSelected = (NewState == ECheckBoxState::Checked);
}

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------
FSlateColor SAIAssetNamerTab::GetStatusColor(const FString& Status) const
{
	if (Status == TEXT("Applied"))   return FSlateColor(FLinearColor(1.0f, 0.85f, 0.3f));
	if (Status == TEXT("Suggested")) return FSlateColor(FLinearColor(0.4f, 1.0f, 0.6f));
	if (Status == TEXT("Skipped"))   return FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f));
	return FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)); // Pending
}

FText SAIAssetNamerTab::GetStatusText() const
{
	return StatusTextBlock.IsValid() ? StatusTextBlock->GetText() : FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
