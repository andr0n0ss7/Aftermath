// Aftermath — game mode, the level's case director, and the greybox HUD (replaced by UMG/CommonUI in Phase 2).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Actor.h"
#include "AMGameFramework.generated.h"

class UAMCaseData;

UCLASS()
class AFTERMATHCORE_API AAMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAMGameMode();
	/** First launch of a packaged game: benchmark the PC and pick Low/Medium/High automatically. */
	virtual void StartPlay() override;
};

/** Place one in each case level. Starts its case when play begins. */
UCLASS()
class AFTERMATHCORE_API AAMCaseDirector : public AActor
{
	GENERATED_BODY()

public:
	AAMCaseDirector();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case") TObjectPtr<UAMCaseData> Case;

protected:
	virtual void BeginPlay() override;
};

UCLASS()
class AFTERMATHCORE_API AAMDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	UFUNCTION() void HandleMessage(const FString& Message);

	void Line(const FString& Text, float X, float& Y, const FLinearColor& Color, float Scale = 1.f);
	void WrapLines(const FString& Text, float X, float& Y, const FLinearColor& Color);

	FString LastMessage;
	double LastMessageTime = -100.0;
};
