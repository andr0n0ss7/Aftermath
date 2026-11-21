// Aftermath — runs one case in the world: spawns traces, applies tool actions, keeps strings and figures, scores the report.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AMTypes.h"
#include "AMCaseSubsystem.generated.h"

class UAMCaseData;
class AAMEvidenceActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAMOnMessage, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAMOnStateChanged);

UCLASS()
class AFTERMATHCORE_API UAMCaseSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "Aftermath") void StartCase(UAMCaseData* InCase);

	// Tool actions. Each returns false and broadcasts a message when it cannot be done.
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool Document(AAMEvidenceActor* E, FVector ViewerLocation);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool ToggleExcluded(AAMEvidenceActor* E);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool Measure(AAMEvidenceActor* E);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool TestLead(AAMEvidenceActor* E);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool TestCopper(AAMEvidenceActor* E);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool PlaceRod(AAMEvidenceActor* E);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool RecoverProjectile(AAMEvidenceActor* E);
	/** Runs a string from the defect. In assisted mode the angles come from the rod or the caliper reading. */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool RunString(AAMEvidenceActor* E);
	/** Sets a string's angles directly (expert mode / fine tuning). */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool SetStringAngles(AAMEvidenceActor* E, float VerticalDeg, float HorizontalDeg);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool RemoveString(AAMEvidenceActor* E);
	/** Cycles the entry / exit / ricochet / not-a-bullet call (glass: which side the shot came from). */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool CycleCall(AAMEvidenceActor* E);
	/** Glass: loupe and side light on the hole's edge; shows which face the crater widens on. */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool ExamineEdge(AAMEvidenceActor* E);
	/** Firearms: record and photograph the condition as found, hands off. Must come before MakeSafe. */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool RecordCondition(AAMEvidenceActor* E);
	/** Firearms: unload and make safe; reveals magazine, chamber or cylinder contents. */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool MakeSafe(AAMEvidenceActor* E);
	/** Collects, packages, seals and marks an item. Only after it is marked and photographed in place. */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool Package(AAMEvidenceActor* E);
	/** Sends a packaged item to the crime lab. Uses one of the case's lab requests. */
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool SubmitToLab(AAMEvidenceActor* E);
	/** Lab report for an item: empty if never submitted, a waiting note if not back yet. */
	UFUNCTION(BlueprintPure, Category = "Aftermath|Tools") FString GetLabResult(const AAMEvidenceActor* E) const;
	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") int32 LabRequestsLeft = 0;
	/** Direction the bullet was travelling when it made this trace (after any ricochet). */
	FVector ShotDirectionFor(const AAMEvidenceActor* E) const;
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") int32 PlaceFigure(FVector FloorPoint, float FacingYawDeg);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool SetFigure(int32 Number, const FString& Identity, EAMPosture Posture);
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Tools") bool RemoveFigure(int32 Number);

	UFUNCTION(BlueprintCallable, Category = "Aftermath|Report") FAMScoreResult SubmitReport();
	UFUNCTION(BlueprintCallable, Category = "Aftermath|Report") FString BuildReportText() const;

	/** Muzzle height of a figure for its posture, cm. */
	static float MuzzleHeightCm(EAMPosture Posture) { return Posture == EAMPosture::Standing ? 142.f : 98.f; }

	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") TObjectPtr<UAMCaseData> Case;
	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") TArray<TObjectPtr<AAMEvidenceActor>> Evidence;
	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") TArray<FAMTrajectoryString> Strings;
	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") TArray<FAMFigure> Figures;
	UPROPERTY(BlueprintReadWrite, Category = "Aftermath") FAMClaims Claims;
	UPROPERTY(BlueprintReadWrite, Category = "Aftermath") bool bAssisted = true;
	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") int32 SwabsLeft = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") bool bSubmitted = false;
	UPROPERTY(BlueprintReadOnly, Category = "Aftermath") FAMScoreResult LastScore;
	UPROPERTY(BlueprintReadWrite, Category = "Aftermath") bool bShowTruth = false;

	UPROPERTY(BlueprintAssignable) FAMOnMessage OnMessage;
	UPROPERTY(BlueprintAssignable) FAMOnStateChanged OnStateChanged;

	int32 CountDocumented() const;
	AAMEvidenceActor* FindEvidenceActor(const FString& Id) const;

private:
	void Say(const FString& Message);
	void Changed();
	bool IsJacketed(const FAMEvidenceDef& Def) const;
	void RebuildStringVisual(FAMTrajectoryString& S);
	FAMTrajectoryString* FindString(const FString& EvidenceId);
	void DrawOverlays() const;

	int32 NextMarker = 1;
	int32 NextFigure = 1;
};
