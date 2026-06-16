#include "ProjectDoctorModule.h"
#include "SProjectDoctorTab.h"

#include "Framework/Docking/TabManager.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/CoreStyle.h"
#include "Interfaces/IPluginManager.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "ProjectDoctor"

static const FName ProjectDoctorTabId("ProjectDoctorTab");

// -------------------------------------------------------------------------
// Commands
// -------------------------------------------------------------------------
FProjectDoctorCommands::FProjectDoctorCommands()
	: TCommands<FProjectDoctorCommands>(
		TEXT("ProjectDoctor"),
		LOCTEXT("ProjectDoctor", "Project Doctor"),
		NAME_None,
		FCoreStyle::Get().GetStyleSetName())
{
}

void FProjectDoctorCommands::RegisterCommands()
{
	UI_COMMAND(OpenTab,  "Project Doctor",  "Open the Project Doctor panel.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ScanGame, "Scan /Game",      "Run Project Doctor on /Game.",    EUserInterfaceActionType::Button, FInputChord());
}

// -------------------------------------------------------------------------
// Module
// -------------------------------------------------------------------------
void FProjectDoctorModule::StartupModule()
{
	RegisterStyle();
	FProjectDoctorCommands::Register();

	CommandList = MakeShared<FUICommandList>();
	CommandList->MapAction(
		FProjectDoctorCommands::Get().OpenTab,
		FExecuteAction::CreateLambda([this]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(ProjectDoctorTabId);
		})
	);

	// Register dockable tab
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ProjectDoctorTabId,
		FOnSpawnTab::CreateRaw(this, &FProjectDoctorModule::SpawnTab))
		.SetDisplayName(LOCTEXT("TabTitle", "Project Doctor"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Asset quality scanner for your Unreal project."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(StyleSet->GetStyleSetName(), "ProjectDoctor.TabIcon"));

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FProjectDoctorModule::RegisterMenus));
}

void FProjectDoctorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FProjectDoctorCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectDoctorTabId);

	UnregisterStyle();
}

TSharedRef<SDockTab> FProjectDoctorModule::SpawnTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SProjectDoctorTab)
		];

	WeakTab = Tab;
	return Tab;
}

void FProjectDoctorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// Tools menu entry
	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection("ProjectDoctorSection");
	Section.Label = LOCTEXT("SectionLabel", "Project Doctor");
	Section.AddMenuEntryWithCommandList(FProjectDoctorCommands::Get().OpenTab, CommandList);
	Section.AddMenuEntryWithCommandList(FProjectDoctorCommands::Get().ScanGame, CommandList);
}

void FProjectDoctorModule::RegisterStyle()
{
	StyleSet = MakeShared<FSlateStyleSet>("ProjectDoctorStyle");

	const FString PluginDir = IPluginManager::Get().FindPlugin(TEXT("ProjectDoctor"))->GetBaseDir();
	StyleSet->SetContentRoot(PluginDir / TEXT("Resources"));

	// Tab icon — falls back gracefully if PNG is missing
	StyleSet->Set("ProjectDoctor.TabIcon",
		new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Icon128.png")), FVector2D(16.f, 16.f)));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FProjectDoctorModule::UnregisterStyle()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectDoctorModule, ProjectDoctor)
