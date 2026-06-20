#include "AIAssetNamerModule.h"
#include "SAIAssetNamerTab.h"
#include "AIAssetNamerSettings.h"

#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FAIAssetNamerModule"

static const FName TabName("AIAssetNamer");

void FAIAssetNamerModule::StartupModule()
{
	// Register icon style
	StyleSet = MakeShareable(new FSlateStyleSet("AIAssetNamerStyle"));
	FString ResourcesDir = IPluginManager::Get().FindPlugin(TEXT("AIAssetNamer"))->GetBaseDir() / TEXT("Resources");
	StyleSet->SetContentRoot(ResourcesDir);
	StyleSet->Set("AIAssetNamer.TabIcon", new FSlateImageBrush(ResourcesDir / TEXT("icon128.png"), FVector2D(40, 40)));
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Editor", "Plugins", "AIAssetNamer",
			LOCTEXT("SettingsName", "AI Asset Namer"),
			LOCTEXT("SettingsDesc", "Configure your API key and AI model for AI Asset Namer."),
			GetMutableDefault<UAIAssetNamerSettings>()
		);
	}

	// Register nomad tab
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName,
		FOnSpawnTab::CreateRaw(this, &FAIAssetNamerModule::SpawnTab))
		.SetDisplayName(LOCTEXT("TabTitle", "AI Asset Namer"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Rename assets automatically using AI"))
		.SetIcon(FSlateIcon("AIAssetNamerStyle", "AIAssetNamer.TabIcon"))
		;

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAIAssetNamerModule::RegisterMenus));
}

void FAIAssetNamerModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "Plugins", "AIAssetNamer");
	}

	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}
}

void FAIAssetNamerModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("Tools");
	Section.AddMenuEntry(
		"OpenAIAssetNamer",
		LOCTEXT("MenuLabel", "AI Asset Namer"),
		LOCTEXT("MenuTooltip", "Open the AI Asset Namer panel"),
		FSlateIcon("AIAssetNamerStyle", "AIAssetNamer.TabIcon"),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId("AIAssetNamer"));
		}))
	);
}

TSharedRef<SDockTab> FAIAssetNamerModule::SpawnTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SAIAssetNamerTab)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAIAssetNamerModule, AIAssetNamer)
