// Aftermath — one trace in the scene: its look, its pick volume, and what the player has done to it.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AMTypes.h"
#include "AMEvidenceActor.generated.h"

class USphereComponent;
class UDecalComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class AFTERMATHCORE_API AAMEvidenceActor : public AActor
{
	GENERATED_BODY()

public:
	AAMEvidenceActor();

	/** Sets up look and pick volume from the case definition. Called right after spawning. */
	void InitFromDef(const FAMEvidenceDef& InDef);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Evidence") FAMEvidenceDef Def;

	// Player state
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") int32 MarkerNumber = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bExcluded = false;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") FAMMeasurement Measurement;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") EAMTestResult LeadResult = EAMTestResult::NotTested;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") EAMTestResult CopperResult = EAMTestResult::NotTested;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bRodPlaced = false;
	/** Set when a rod is placed: unit vector back toward the shooter, with angle-finder noise. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") FVector RodBackDirection = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bProjectileRecovered = false;
	/** The player's entry / exit / ricochet / not-a-bullet call (or, for glass, which side the shot came from). */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") EAMDefectCall Call = EAMDefectCall::Unclassified;
	/** Glass: edge and crater examined under magnification and side light. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bEdgeExamined = false;
	/** Firearms: visible condition recorded and photographed before anything was touched. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bConditionRecorded = false;
	/** Firearms: unloaded and made safe; contents now known. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bMadeSafe = false;
	/** Firearms: made safe before the condition was recorded, so the as-found state is lost. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bConditionLost = false;
	/** Collected, packaged, sealed and marked (container marked, never the bullet or case itself). */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bPackaged = false;
	/** Sent to the crime lab; result available once the world time passes LabReadyTime. */
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bSubmittedToLab = false;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") double LabReadyTime = 0.0;
	UPROPERTY(BlueprintReadOnly, Category = "Evidence") bool bLabResultAnnounced = false;

	UFUNCTION(BlueprintPure, Category = "Evidence") bool IsDocumented() const { return MarkerNumber > 0; }
	UFUNCTION(BlueprintPure, Category = "Evidence") bool IsInReport() const { return MarkerNumber > 0 && !bExcluded; }
	UFUNCTION(BlueprintPure, Category = "Evidence") bool IsSurfaceDefect() const;
	UFUNCTION(BlueprintPure, Category = "Evidence") bool CanTakeString() const;
	UFUNCTION(BlueprintPure, Category = "Evidence") bool IsWeapon() const;
	/** Surface defects and glass take an entry / exit / ricochet call. */
	UFUNCTION(BlueprintPure, Category = "Evidence") bool IsCallable() const;
	/** Order the call key cycles through for this kind of trace. */
	TArray<EAMDefectCall> CallOptions() const;
	/** What the right call is. Glass needs the shot's direction of travel. */
	EAMDefectCall TruthCall(const FVector& ShotDirection) const;
	FString CallLabel(EAMDefectCall InCall) const;

	/** Glass: what the edge examination shows (crater and bevel on the exit face). */
	FString GetEdgeFinding(const FVector& ShotDirection) const;
	/** Firearms: visible condition (no handling). */
	TArray<FString> GetConditionLines() const;
	/** Firearms: contents found while unloading. */
	TArray<FString> GetUnloadLines() const;
	/** Whether this item can be collected and packaged at all (walls and glass stay in place). */
	bool IsCollectable() const;
	/** How it is packaged and marked, following field practice for its kind. */
	FString GetPackagingNote() const;

	/** What the player sees before documenting it. */
	UFUNCTION(BlueprintPure, Category = "Evidence") FString GetShortLabel() const;
	UFUNCTION(BlueprintPure, Category = "Evidence") FString GetMaterialLabel() const;
	UFUNCTION(BlueprintPure, Category = "Evidence") FString GetObservation() const;
	UFUNCTION(BlueprintPure, Category = "Evidence") FString GetProjectileDescription() const;

	/** Physical test outcomes. Answers come from the trace itself, so decoys test negative. */
	EAMTestResult EvaluateLead() const;
	EAMTestResult EvaluateCopper(bool bShotWasJacketed) const;

	/** Shows or hides the numbered marker. */
	void SetMarker(int32 Number, const FVector& ViewerLocation);
	void SetExcluded(bool bInExcluded);

	/** Size of the close-up field and the decal, in cm. */
	static float FieldCm(EAMEvidenceKind Kind);
	static float PickRadiusCm(EAMEvidenceKind Kind);

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USphereComponent> PickVolume;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UDecalComponent> Decal;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Prop;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UTextRenderComponent> Marker;
	/** Firearms: magazine (pistol) or cylinder (revolver) as a separate part. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Part;

	/** Firearms: moves the magazine out beside the gun, or swings the cylinder out, after making safe. */
	void ShowMadeSafe();
};
