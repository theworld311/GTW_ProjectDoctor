#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/Commands.h"
#include "Styling/SlateStyle.h"

class FProjectDoctorCommands : public TCommands<FProjectDoctorCommands>
{
public:
	FProjectDoctorCommands();
	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> OpenTab;
	TSharedPtr<FUICommandInfo> ScanGame;
};

class FProjectDoctorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FProjectDoctorModule& Get()
	{
		return FModuleManager::GetModuleChecked<FProjectDoctorModule>("ProjectDoctor");
	}

	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);

private:
	void RegisterMenus();
	void RegisterStyle();
	void UnregisterStyle();

	TSharedPtr<FUICommandList> CommandList;
	TSharedPtr<FSlateStyleSet> StyleSet;
	TWeakPtr<SDockTab> WeakTab;
};
