#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIAssetNamerSettings.generated.h"

UENUM()
enum class EAIProvider : uint8
{
	Claude   UMETA(DisplayName = "Claude (Anthropic)"),
	OpenAI   UMETA(DisplayName = "OpenAI (GPT-4o)"),
};

UCLASS(Config = EditorPerProjectUserSettings, defaultconfig)
class UAIAssetNamerSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "AI Asset Namer")
	EAIProvider Provider = EAIProvider::Claude;

	UPROPERTY(Config, EditAnywhere, Category = "AI Asset Namer")
	FString ApiKey;

	UPROPERTY(Config, EditAnywhere, Category = "AI Asset Namer")
	FString ClaudeModel = TEXT("claude-haiku-4-5-20251001");

	UPROPERTY(Config, EditAnywhere, Category = "AI Asset Namer")
	FString OpenAIModel = TEXT("gpt-4o-mini");

	static UAIAssetNamerSettings* Get()
	{
		return GetMutableDefault<UAIAssetNamerSettings>();
	}
};
