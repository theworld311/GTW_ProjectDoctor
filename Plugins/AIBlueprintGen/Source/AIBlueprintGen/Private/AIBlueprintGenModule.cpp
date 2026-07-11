// Copyright (c) 2026 GTW Dev. All Rights Reserved.

#include "AIBlueprintGenModule.h"
#include "SAIBlueprintGenTab.h"
#include "AIBlueprintGenSettings.h"

#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FAIBlueprintGenModule"

static const FName TabName("AIBlueprintGen");

void FAIBlueprintGenModule::StartupModule()
{
	// Register icon style
	StyleSet = MakeShareable(new FSlateStyleSet("AIBlueprintGenStyle"));
	FString ResourcesDir = IPluginManager::Get().FindPlugin(TEXT("AIBlueprintGen"))->GetBaseDir() / TEXT("Resources");
	StyleSet->SetContentRoot(ResourcesDir);
	StyleSet->Set("AIBlueprintGen.TabIcon", new FSlateImageBrush(ResourcesDir / TEXT("icon128.png"), FVector2D(40, 40)));
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Editor", "Plugins", "AIBlueprintGen",
			LOCTEXT("SettingsName", "AI Blueprint Generator"),
			LOCTEXT("SettingsDesc", "Configure your API key and AI model for AI Blueprint Generator."),
			GetMutableDefault<UAIBlueprintGenSettings>()
		);
	}

	// Register nomad tab
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName,
		FOnSpawnTab::CreateRaw(this, &FAIBlueprintGenModule::SpawnTab))
		.SetDisplayName(LOCTEXT("TabTitle", "AI Blueprint Generator"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Generate Blueprints from natural language using AI"))
		.SetIcon(FSlateIcon("AIBlueprintGenStyle", "AIBlueprintGen.TabIcon"));

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAIBlueprintGenModule::RegisterMenus));
}

void FAIBlueprintGenModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "Plugins", "AIBlueprintGen");
	}

	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
		StyleSet.Reset();
	}
}

void FAIBlueprintGenModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("Tools");
	Section.AddMenuEntry(
		"OpenAIBlueprintGen",
		LOCTEXT("MenuLabel", "AI Blueprint Generator"),
		LOCTEXT("MenuTooltip", "Generate Blueprints from natural language using AI"),
		FSlateIcon("AIBlueprintGenStyle", "AIBlueprintGen.TabIcon"),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId("AIBlueprintGen"));
		}))
	);
}

TSharedRef<SDockTab> FAIBlueprintGenModule::SpawnTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SAIBlueprintGenTab)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAIBlueprintGenModule, AIBlueprintGen)
