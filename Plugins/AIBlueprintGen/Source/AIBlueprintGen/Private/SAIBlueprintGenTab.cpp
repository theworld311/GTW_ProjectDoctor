// Copyright (c) 2026 GTW Dev. All Rights Reserved.

#include "SAIBlueprintGenTab.h"
#include "AIBlueprintGenSettings.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "IPythonScriptPlugin.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "SAIBlueprintGenTab"

void SAIBlueprintGenTab::Construct(const FArguments& InArgs)
{
	// Provider options
	ProviderOptions.Add(MakeShareable(new FString(TEXT("Claude (Anthropic)"))));
	ProviderOptions.Add(MakeShareable(new FString(TEXT("OpenAI (GPT-4o)"))));
	SelectedProvider = ProviderOptions[0];

	// Parent class options
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("Actor"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("Character"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("Pawn"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("GameMode"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("GameInstance"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("PlayerController"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("ActorComponent"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("SceneComponent"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("UserWidget"))));
	ParentClassOptions.Add(MakeShareable(new FString(TEXT("AnimInstance"))));
	SelectedParentClass = ParentClassOptions[0];

	TSharedPtr<SVerticalBox> VBox = SNew(SVerticalBox);

	// Settings bar
	VBox->AddSlot().AutoHeight().Padding(6, 6, 6, 2)
	[
		BuildSettingsBar()
	];

	VBox->AddSlot().AutoHeight()
	[
		SNew(SSeparator)
	];

	// Input area
	VBox->AddSlot().FillHeight(0.45f).Padding(6, 4)
	[
		BuildInputArea()
	];

	VBox->AddSlot().AutoHeight()
	[
		SNew(SSeparator)
	];

	// Output area
	VBox->AddSlot().FillHeight(0.55f).Padding(6, 4, 6, 2)
	[
		BuildOutputArea()
	];

	// Status bar
	VBox->AddSlot().AutoHeight().Padding(6, 2, 6, 6)
	[
		SAssignNew(StatusBlock, STextBlock)
		.Text(LOCTEXT("StatusReady", "Ready. Describe your Blueprint and click Generate."))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
	];

	ChildSlot[ VBox.ToSharedRef() ];
}

// -------------------------------------------------------------------------
// Settings bar
// -------------------------------------------------------------------------
TSharedRef<SWidget> SAIBlueprintGenTab::BuildSettingsBar()
{
	UAIBlueprintGenSettings* Settings = UAIBlueprintGenSettings::Get();

	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
	[
		SNew(STextBlock).Text(LOCTEXT("LblProvider", "Provider:"))
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
			SNew(STextBlock).Text_Lambda([this]()
			{
				return SelectedProvider.IsValid() ? FText::FromString(*SelectedProvider) : FText::GetEmpty();
			})
		]
	]

	+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
	[
		SNew(STextBlock).Text(LOCTEXT("LblApiKey", "API Key:"))
	]
	+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 4, 0)
	[
		SAssignNew(ApiKeyBox, SEditableTextBox)
		.Text(FText::FromString(Settings->ApiKey))
		.IsPassword(true)
		.HintText(LOCTEXT("ApiKeyHint", "Paste your API key here..."))
		.OnTextCommitted_Lambda([](const FText& Text, ETextCommit::Type)
		{
			UAIBlueprintGenSettings* S = UAIBlueprintGenSettings::Get();
			S->ApiKey = Text.ToString();
			S->SaveConfig();
		})
	]
	+ SHorizontalBox::Slot().AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("BtnSave", "Save"))
		.OnClicked_Lambda([this]() -> FReply
		{
			UAIBlueprintGenSettings* S = UAIBlueprintGenSettings::Get();
			S->ApiKey = ApiKeyBox->GetText().ToString();
			S->SaveConfig();
			StatusBlock->SetText(LOCTEXT("KeySaved", "API key saved."));
			return FReply::Handled();
		})
	];
}

// -------------------------------------------------------------------------
// Input area
// -------------------------------------------------------------------------
TSharedRef<SWidget> SAIBlueprintGenTab::BuildInputArea()
{
	return SNew(SVerticalBox)

	// Title
	+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("InputTitle", "Describe your Blueprint"))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
	]

	// Options row
	+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("LblParentClass", "Parent Class:"))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 16, 0)
		[
			SAssignNew(ParentClassCombo, SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&ParentClassOptions)
			.InitiallySelectedItem(SelectedParentClass)
			.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
			{
				SelectedParentClass = Item;
			})
			.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
			{
				return SNew(STextBlock).Text(FText::FromString(*Item));
			})
			[
				SNew(STextBlock).Text_Lambda([this]()
				{
					return SelectedParentClass.IsValid() ? FText::FromString(*SelectedParentClass) : FText::GetEmpty();
				})
			]
		]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
		[
			SNew(STextBlock).Text(LOCTEXT("LblFolder", "Save to:"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 8, 0)
		[
			SAssignNew(FolderPathBox, SEditableTextBox)
			.Text(FText::FromString(TEXT("/Game/Blueprints")))
			.HintText(LOCTEXT("FolderHint", "/Game/Blueprints"))
		]
	]

	// Description text box
	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f))
		.Padding(4)
		[
			SAssignNew(DescriptionBox, SMultiLineEditableTextBox)
			.HintText(LOCTEXT("DescHint",
				"Example: Create a health system for a Character with:\n"
				"- Float variable Health (default 100)\n"
				"- Float variable MaxHealth (default 100)\n"
				"- Function TakeDamage that reduces Health\n"
				"- Auto-regeneration of 5 health per second after 3 seconds without damage\n"
				"- Event OnDeath when Health reaches 0"))
			.AutoWrapText(true)
		]
	]

	// Generate button
	+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0).HAlign(HAlign_Left)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
		[
			SNew(SButton)
			.Text_Lambda([this]() -> FText
			{
				return bRequesting
					? LOCTEXT("BtnGenerating", "Generating...")
					: LOCTEXT("BtnGenerate", "Generate Blueprint (AI)");
			})
			.IsEnabled_Lambda([this]() { return !bRequesting; })
			.OnClicked(this, &SAIBlueprintGenTab::OnGenerateClicked)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("BtnClear", "Clear"))
			.OnClicked(this, &SAIBlueprintGenTab::OnClearClicked)
		]
	];
}

// -------------------------------------------------------------------------
// Output area
// -------------------------------------------------------------------------
TSharedRef<SWidget> SAIBlueprintGenTab::BuildOutputArea()
{
	return SNew(SVerticalBox)

	+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("OutputTitle", "Generated Python Script"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("BtnCopy", "Copy Script"))
			.OnClicked(this, &SAIBlueprintGenTab::OnCopyClicked)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
		[
			SNew(SButton)
			.Text(LOCTEXT("BtnExecute", "Create Blueprint in Editor"))
			.OnClicked(this, &SAIBlueprintGenTab::OnExecuteClicked)
		]
	]

	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.03f, 0.05f, 0.03f))
		.Padding(4)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(OutputBox, SMultiLineEditableTextBox)
				.IsReadOnly(false)
				.AutoWrapText(false)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 9))
				.HintText(LOCTEXT("OutputHint", "The generated Python script will appear here..."))
			]
		]
	];
}

// -------------------------------------------------------------------------
// Generate
// -------------------------------------------------------------------------
FReply SAIBlueprintGenTab::OnGenerateClicked()
{
	FString Description = DescriptionBox->GetText().ToString().TrimStartAndEnd();
	if (Description.IsEmpty())
	{
		StatusBlock->SetText(LOCTEXT("NoDesc", "Please describe the Blueprint you want to create."));
		return FReply::Handled();
	}

	UAIBlueprintGenSettings* Settings = UAIBlueprintGenSettings::Get();
	Settings->ApiKey = ApiKeyBox->GetText().ToString();
	Settings->SaveConfig();

	if (Settings->ApiKey.IsEmpty())
	{
		StatusBlock->SetText(LOCTEXT("NoKey", "Please enter your API key first."));
		return FReply::Handled();
	}

	bRequesting = true;
	StatusBlock->SetText(LOCTEXT("Sending", "Sending to AI... Please wait."));
	OutputBox->SetText(FText::GetEmpty());

	SendToAI();
	return FReply::Handled();
}

// -------------------------------------------------------------------------
// System prompt — teaches the AI exactly what Python API to use
// -------------------------------------------------------------------------
FString SAIBlueprintGenTab::BuildSystemPrompt() const
{
	return TEXT(
		"You are an expert Unreal Engine 5 Python automation engineer.\n"
		"Your job is to generate Python scripts that create Blueprints using the Unreal Engine Python API.\n\n"
		"RULES:\n"
		"1. Always output ONLY a Python script, no explanation before or after.\n"
		"2. Wrap the script in ```python ... ``` code block.\n"
		"3. Use only these Unreal Engine Python APIs:\n"
		"   - unreal.AssetToolsHelpers.get_asset_tools()\n"
		"   - unreal.EditorAssetLibrary\n"
		"   - unreal.load_asset(path)\n"
		"   - unreal.Blueprint\n"
		"   - unreal.BlueprintEditorLibrary\n"
		"   - unreal.KismetSystemLibrary\n"
		"   - asset_tools.create_asset(name, path, asset_class, factory)\n"
		"   - unreal.BlueprintFactory()\n"
		"   - unreal.AnimBlueprintFactory()\n"
		"   - unreal.WidgetBlueprintFactory()\n"
		"4. To add variables to a Blueprint use:\n"
		"   bp_obj = unreal.load_asset(full_path)\n"
		"   var = unreal.BPVariableDescription()\n"
		"   var.set_editor_property('var_name', 'VariableName')\n"
		"   var.set_editor_property('var_type', unreal.EdGraphPinType(...))\n"
		"   unreal.BlueprintEditorLibrary.add_member_variable(bp_obj, 'VariableName', unreal.EdGraphPinType(pc_real=unreal.EPinContainerType.NONE, pc_sub_category_object=None))\n"
		"5. To add a simple float variable use:\n"
		"   unreal.BlueprintEditorLibrary.add_member_variable(bp_obj, 'Health', unreal.EdGraphPinType(pc_category='real', pc_sub_category='double', pc_sub_category_object=None, pc_is_array=False, pc_is_reference=False, pc_is_weak_reference=False, pc_is_map=False, pc_is_set=False))\n"
		"6. Always save assets with: unreal.EditorAssetLibrary.save_asset(full_path)\n"
		"7. Always compile blueprints with: unreal.BlueprintEditorLibrary.compile_blueprint(bp_obj)\n"
		"8. Print status messages so the user knows what happened.\n"
		"9. The script must be self-contained and runnable.\n"
		"10. Name the Blueprint with correct UE5 prefix (BP_, ABP_, WBP_) based on parent class.\n"
	);
}

FString SAIBlueprintGenTab::BuildUserPrompt() const
{
	FString ParentClass = SelectedParentClass.IsValid() ? *SelectedParentClass : TEXT("Actor");
	FString FolderPath  = FolderPathBox.IsValid() ? FolderPathBox->GetText().ToString() : TEXT("/Game/Blueprints");
	FString Description = DescriptionBox->GetText().ToString();

	return FString::Printf(TEXT(
		"Create an Unreal Engine 5 Blueprint Python script with these specifications:\n\n"
		"Parent Class: %s\n"
		"Save Folder: %s\n"
		"Description:\n%s\n\n"
		"Generate a complete Python script that creates this Blueprint in the Unreal Editor."
	), *ParentClass, *FolderPath, *Description);
}

// -------------------------------------------------------------------------
// HTTP request
// -------------------------------------------------------------------------
void SAIBlueprintGenTab::SendToAI()
{
	UAIBlueprintGenSettings* Settings = UAIBlueprintGenSettings::Get();
	const bool bUseClaude = !SelectedProvider.IsValid() || (*SelectedProvider).Contains(TEXT("Claude"));

	FString RequestBody;
	FString Url;

	const FString SystemPrompt = BuildSystemPrompt();
	const FString UserPrompt   = BuildUserPrompt();

	if (bUseClaude)
	{
		Url = TEXT("https://api.anthropic.com/v1/messages");

		TSharedPtr<FJsonObject> MsgObj = MakeShareable(new FJsonObject());
		MsgObj->SetStringField(TEXT("role"), TEXT("user"));
		MsgObj->SetStringField(TEXT("content"), UserPrompt);

		TArray<TSharedPtr<FJsonValue>> Messages;
		Messages.Add(MakeShareable(new FJsonValueObject(MsgObj)));

		TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject());
		Root->SetStringField(TEXT("model"), Settings->ClaudeModel);
		Root->SetNumberField(TEXT("max_tokens"), 8192);
		Root->SetStringField(TEXT("system"), SystemPrompt);
		Root->SetArrayField(TEXT("messages"), Messages);

		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
		FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	}
	else
	{
		Url = TEXT("https://api.openai.com/v1/chat/completions");

		TSharedPtr<FJsonObject> SysMsg = MakeShareable(new FJsonObject());
		SysMsg->SetStringField(TEXT("role"), TEXT("system"));
		SysMsg->SetStringField(TEXT("content"), SystemPrompt);

		TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject());
		UserMsg->SetStringField(TEXT("role"), TEXT("user"));
		UserMsg->SetStringField(TEXT("content"), UserPrompt);

		TArray<TSharedPtr<FJsonValue>> Messages;
		Messages.Add(MakeShareable(new FJsonValueObject(SysMsg)));
		Messages.Add(MakeShareable(new FJsonValueObject(UserMsg)));

		TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject());
		Root->SetStringField(TEXT("model"), Settings->OpenAIModel);
		Root->SetNumberField(TEXT("max_tokens"), 8192);
		Root->SetArrayField(TEXT("messages"), Messages);

		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
		FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
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
	// BindSP: callback so dispara se a aba ainda existir (evita crash se o usuario fechar durante o pedido)
	Request->OnProcessRequestComplete().BindSP(SharedThis(this), &SAIBlueprintGenTab::OnAIResponseReceived);
	Request->ProcessRequest();
}

// -------------------------------------------------------------------------
// Response handler
// -------------------------------------------------------------------------
void SAIBlueprintGenTab::OnAIResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
	bRequesting = false;

	if (!bSuccess || !Response.IsValid())
	{
		StatusBlock->SetText(LOCTEXT("NetworkError", "Network error. Check your connection."));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		StatusBlock->SetText(FText::Format(
			LOCTEXT("ApiError", "API error {0}. Check your API key."),
			FText::AsNumber(Response->GetResponseCode())
		));
		return;
	}

	const bool bUseClaude = !SelectedProvider.IsValid() || (*SelectedProvider).Contains(TEXT("Claude"));
	FString RawText = ExtractTextFromResponse(Response->GetContentAsString(), bUseClaude);

	if (RawText.IsEmpty())
	{
		StatusBlock->SetText(LOCTEXT("EmptyResponse", "AI returned an empty response."));
		return;
	}

	LastGeneratedScript = ExtractPythonScript(RawText);

	if (LastGeneratedScript.IsEmpty())
	{
		// Show raw response if no code block found
		LastGeneratedScript = RawText;
	}

	OutputBox->SetText(FText::FromString(LastGeneratedScript));
	StatusBlock->SetText(LOCTEXT("GenerateDone", "Script generated! Review it and click 'Create Blueprint in Editor'."));
}

FString SAIBlueprintGenTab::ExtractTextFromResponse(const FString& JsonBody, bool bUseClaude)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonBody);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return TEXT("");

	FString Result;

	if (bUseClaude)
	{
		const TArray<TSharedPtr<FJsonValue>>* ContentArr = nullptr;
		if (Root->TryGetArrayField(TEXT("content"), ContentArr) && ContentArr && ContentArr->Num() > 0)
		{
			const TSharedPtr<FJsonObject>* Block = nullptr;
			if ((*ContentArr)[0]->TryGetObject(Block) && Block)
			{
				(*Block)->TryGetStringField(TEXT("text"), Result);
			}
		}
	}
	else
	{
		const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
		if (Root->TryGetArrayField(TEXT("choices"), Choices) && Choices && Choices->Num() > 0)
		{
			const TSharedPtr<FJsonObject>* Choice = nullptr;
			if ((*Choices)[0]->TryGetObject(Choice))
			{
				const TSharedPtr<FJsonObject>* MsgObj = nullptr;
				if ((*Choice)->TryGetObjectField(TEXT("message"), MsgObj))
				{
					(*MsgObj)->TryGetStringField(TEXT("content"), Result);
				}
			}
		}
	}

	return Result;
}

FString SAIBlueprintGenTab::ExtractPythonScript(const FString& RawText)
{
	// Extract content between ```python and ```
	const FString StartTag = TEXT("```python");
	const FString EndTag   = TEXT("```");

	int32 Start = RawText.Find(StartTag);
	if (Start == INDEX_NONE)
	{
		// Try generic code block
		Start = RawText.Find(TEXT("```"));
		if (Start == INDEX_NONE) return TEXT("");
		Start += 3;
	}
	else
	{
		Start += StartTag.Len();
	}

	// Skip newline after opening tag
	if (Start < RawText.Len() && (RawText[Start] == TEXT('\n') || RawText[Start] == TEXT('\r')))
	{
		Start++;
	}

	int32 End = RawText.Find(EndTag, ESearchCase::IgnoreCase, ESearchDir::FromStart, Start);
	if (End == INDEX_NONE) return RawText.Mid(Start);

	return RawText.Mid(Start, End - Start);
}

// -------------------------------------------------------------------------
// Execute script in editor
// -------------------------------------------------------------------------
FReply SAIBlueprintGenTab::OnExecuteClicked()
{
	// Use whatever is currently in the output box (user may have edited it)
	FString Script = OutputBox->GetText().ToString().TrimStartAndEnd();

	if (Script.IsEmpty())
	{
		StatusBlock->SetText(LOCTEXT("NoScript", "No script to execute. Generate one first."));
		return FReply::Handled();
	}

	StatusBlock->SetText(LOCTEXT("Executing", "Creating Blueprint in editor..."));

	if (IPythonScriptPlugin* Python = IPythonScriptPlugin::Get())
	{
		Python->ExecPythonCommand(*Script);
		StatusBlock->SetText(LOCTEXT("ExecuteDone", "Done! Check your Content Browser for the new Blueprint."));
	}
	else
	{
		StatusBlock->SetText(LOCTEXT("NoPython", "Python Script Plugin not available. Enable it in Edit > Plugins."));
	}

	return FReply::Handled();
}

// -------------------------------------------------------------------------
// Copy script to clipboard
// -------------------------------------------------------------------------
FReply SAIBlueprintGenTab::OnCopyClicked()
{
	FString Script = OutputBox->GetText().ToString();
	if (!Script.IsEmpty())
	{
		FPlatformApplicationMisc::ClipboardCopy(*Script);
		StatusBlock->SetText(LOCTEXT("Copied", "Script copied to clipboard."));
	}
	return FReply::Handled();
}

// -------------------------------------------------------------------------
// Clear
// -------------------------------------------------------------------------
FReply SAIBlueprintGenTab::OnClearClicked()
{
	DescriptionBox->SetText(FText::GetEmpty());
	OutputBox->SetText(FText::GetEmpty());
	LastGeneratedScript = TEXT("");
	StatusBlock->SetText(LOCTEXT("Cleared", "Ready."));
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
