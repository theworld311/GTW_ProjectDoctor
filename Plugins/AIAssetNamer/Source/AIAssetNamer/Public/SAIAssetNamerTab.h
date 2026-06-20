#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Interfaces/IHttpRequest.h"

// -------------------------------------------------------------------------
// One asset suggestion row
// -------------------------------------------------------------------------
struct FAIAssetSuggestion
{
	FString AssetPath;      // /Game/Characters/myChar
	FString AssetClass;     // StaticMesh, Texture2D, etc.
	FString CurrentName;    // myChar
	FString SuggestedName;  // SM_Char  (filled after AI reply)
	FString Status;         // "Pending" | "Suggested" | "Applied" | "Skipped"
	bool    bSelected = true;
};

using FAIAssetSuggestionPtr = TSharedPtr<FAIAssetSuggestion>;

// -------------------------------------------------------------------------
// Main widget
// -------------------------------------------------------------------------
class SAIAssetNamerTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAIAssetNamerTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// ---- UI builders ----
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildSettingsBar();
	TSharedRef<SWidget> BuildSummaryBar();
	TSharedRef<ITableRow> GenerateRow(FAIAssetSuggestionPtr Item, const TSharedRef<STableViewBase>& Owner);

	// ---- Actions ----
	FReply OnLoadSelectedClicked();
	FReply OnLoadFolderClicked();
	FReply OnSuggestNamesClicked();
	FReply OnApplySelectedClicked();
	FReply OnClearClicked();
	void   OnRowCheckChanged(ECheckBoxState NewState, FAIAssetSuggestionPtr Item);

	// ---- AI HTTP ----
	void   SendBatchToAI();
	void   OnAIResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void   ParseAIResponse(const FString& JsonBody);

	// ---- Helpers ----
	FText  GetStatusText() const;
	FSlateColor GetStatusColor(const FString& Status) const;
	FString BuildPrompt() const;

	// ---- State ----
	TArray<FAIAssetSuggestionPtr> Suggestions;

	TSharedPtr<SListView<FAIAssetSuggestionPtr>> ListView;
	TSharedPtr<SEditableTextBox> ApiKeyBox;
	TSharedPtr<STextBlock>       StatusTextBlock;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ProviderCombo;

	TArray<TSharedPtr<FString>> ProviderOptions;
	TSharedPtr<FString>         SelectedProvider;

	bool bRequesting = false;
	int32 PendingRequests = 0;
	int32 CompletedRequests = 0;
};
