#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

// -------------------------------------------------------------------------
// Data model for a single finding
// -------------------------------------------------------------------------
struct FProjectDoctorFinding
{
	FString Severity;   // "Error" | "Warning" | "Info"
	FString Rule;
	FString AssetPath;
	FString Message;
};

using FProjectDoctorFindingPtr = TSharedPtr<FProjectDoctorFinding>;

// -------------------------------------------------------------------------
// Main dockable tab widget
// -------------------------------------------------------------------------
class SProjectDoctorTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectDoctorTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Trigger a scan via Python and reload results
	void RunScan(const FString& RootPath, int32 MaxAssets = 5000);

	// Reload the latest JSON report from disk
	void ReloadReport();

private:
	// ---- UI builders ----
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildFilterBar();
	TSharedRef<SWidget> BuildSummaryBar();
	TSharedRef<ITableRow> GenerateRow(FProjectDoctorFindingPtr Item, const TSharedRef<STableViewBase>& OwnerTable);

	// ---- Actions ----
	FReply OnScanGameClicked();
	FReply OnScanCustomClicked();
	FReply OnOpenHtmlClicked();
	FReply OnRefreshClicked();
	FReply OnClearClicked();
	void   OnFilterChanged(FName Severity);
	void   OnFindingDoubleClicked(FProjectDoctorFindingPtr Item);

	// ---- Helpers ----
	FString GetLatestJsonPath() const;
	void    ApplyFilter();
	FSlateColor GetSeverityColor(const FString& Severity) const;
	FText   GetStatusText() const;
	FText   GetSummaryText() const;

	// ---- State ----
	TArray<FProjectDoctorFindingPtr> AllFindings;
	TArray<FProjectDoctorFindingPtr> FilteredFindings;
	FName ActiveFilter = NAME_None; // NAME_None = All

	int32 TotalErrors   = 0;
	int32 TotalWarnings = 0;
	int32 TotalInfo     = 0;
	int32 ScannedAssets = 0;
	FString LastRootPath;
	FString LastHtmlPath;
	bool bScanning = false;

	TSharedPtr<SListView<FProjectDoctorFindingPtr>> ListView;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<STextBlock> SummaryText;
};
