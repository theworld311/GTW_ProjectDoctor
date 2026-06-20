#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Interfaces/IHttpRequest.h"

class SAIBlueprintGenTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIBlueprintGenTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// ---- UI builders ----
	TSharedRef<SWidget> BuildSettingsBar();
	TSharedRef<SWidget> BuildInputArea();
	TSharedRef<SWidget> BuildOutputArea();

	// ---- Actions ----
	FReply OnGenerateClicked();
	FReply OnExecuteClicked();
	FReply OnClearClicked();
	FReply OnCopyClicked();

	// ---- AI HTTP ----
	void SendToAI();
	void OnAIResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	FString ExtractTextFromResponse(const FString& JsonBody, bool bUseClaude);
	FString ExtractPythonScript(const FString& RawText);
	FString BuildSystemPrompt() const;
	FString BuildUserPrompt() const;

	// ---- Helpers ----
	FText GetStatusText() const;
	FText GetGenerateButtonText() const;

	// ---- Widgets ----
	TSharedPtr<SMultiLineEditableTextBox> DescriptionBox;
	TSharedPtr<SMultiLineEditableTextBox> OutputBox;
	TSharedPtr<SEditableTextBox>          ApiKeyBox;
	TSharedPtr<SEditableTextBox>          FolderPathBox;
	TSharedPtr<STextBlock>                StatusBlock;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ParentClassCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ProviderCombo;

	TArray<TSharedPtr<FString>> ParentClassOptions;
	TSharedPtr<FString>         SelectedParentClass;

	TArray<TSharedPtr<FString>> ProviderOptions;
	TSharedPtr<FString>         SelectedProvider;

	// ---- State ----
	FString LastGeneratedScript;
	bool    bRequesting = false;
};
