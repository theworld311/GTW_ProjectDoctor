// Copyright (c) 2026 GTW Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIBlueprintGenSettings.generated.h"

UENUM()
enum class EAIBPProvider : uint8
{
	Claude  UMETA(DisplayName = "Claude (Anthropic)"),
	OpenAI  UMETA(DisplayName = "OpenAI (GPT-4o)"),
};

UCLASS(Config = EditorPerProjectUserSettings, defaultconfig)
class UAIBlueprintGenSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "AI Blueprint Generator")
	EAIBPProvider Provider = EAIBPProvider::Claude;

	UPROPERTY(Config, EditAnywhere, Category = "AI Blueprint Generator")
	FString ApiKey;

	UPROPERTY(Config, EditAnywhere, Category = "AI Blueprint Generator")
	FString ClaudeModel = TEXT("claude-sonnet-4-6");

	UPROPERTY(Config, EditAnywhere, Category = "AI Blueprint Generator")
	FString OpenAIModel = TEXT("gpt-4o");

	static UAIBlueprintGenSettings* Get()
	{
		return GetMutableDefault<UAIBlueprintGenSettings>();
	}
};
