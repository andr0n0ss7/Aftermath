// Aftermath — the first-person investigator: movement, flashlight, crouch, targeting, and the greybox tool keys.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AMInvestigator.generated.h"

class UCameraComponent;
class USpotLightComponent;
class UInputAction;
class UInputMappingContext;
class AAMEvidenceActor;
class UAMCaseSubsystem;
struct FInputActionValue;

UCLASS()
class AFTERMATHCORE_API AAMInvestigator : public ACharacter
{
	GENERATED_BODY()

public:
	AAMInvestigator();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpotLightComponent> Flashlight;

	/** Trace under the crosshair this frame. */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<AAMEvidenceActor> Focused;
	/** Trace the tool keys act on (last one inspected). */
	UPROPERTY(BlueprintReadOnly) TObjectPtr<AAMEvidenceActor> Selected;
	UPROPERTY(BlueprintReadOnly) bool bReportOpen = false;

	UPROPERTY(EditAnywhere, Category = "Investigator") float ReachCm = 260.f;
	UPROPERTY(EditAnywhere, Category = "Investigator") float WalkSpeed = 150.f;
	UPROPERTY(EditAnywhere, Category = "Investigator") float FastSpeed = 260.f;
	UPROPERTY(EditAnywhere, Category = "Investigator") float LookSensitivity = 1.0f;

	UAMCaseSubsystem* GetCase() const;

	// Console commands for claims and figures until the case board UI exists (Phase 2). Type them with the ~ console.
	UFUNCTION(Exec) void AMShots(int32 Count);
	UFUNCTION(Exec) void AMWeapon(const FString& Key);
	UFUNCTION(Exec) void AMFirst(const FString& ShooterKey);
	UFUNCTION(Exec) void AMStatement(const FString& Verdict);
	UFUNCTION(Exec) void AMFigure(int32 Number, const FString& Identity, const FString& Posture);
	UFUNCTION(Exec) void AMAngles(float VerticalDeg, float HorizontalDeg);
	UFUNCTION(Exec) void AMAssisted(bool bOn);
	UFUNCTION(Exec) void AMTruth(bool bOn);

private:
	void BuildInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void FastOn();
	void FastOff();
	void ToggleCrouch();
	void ToggleFlashlight();
	void Inspect();
	void ToolDocument();
	void ToolMeasure();
	void ToolLead();
	void ToolCopper();
	void ToolRod();
	void ToolRecover();
	void ToolString();
	void ToolExclude();
	void ToolCall();
	void ToolExamine();
	void ToolMakeSafe();
	void ToolPackage();
	void ToolLab();
	void ToolFigure();
	void ToggleReport();
	void Submit();
	bool RequireSelected() const;

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TMap<FName, TObjectPtr<UInputAction>> Actions;
};
