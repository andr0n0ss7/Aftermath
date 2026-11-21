// Aftermath — shared types. One case = one shot record; every other system reads from it.
#pragma once

#include "CoreMinimal.h"
#include "AMTypes.generated.h"

/** Physical kind of a trace. Drives look, close-up text, available tools and test results. */
UENUM(BlueprintType)
enum class EAMEvidenceKind : uint8
{
	DrywallEntry,
	DrywallExit,
	PlasterCrater,
	SteelHole,
	GlassHole,
	CartridgeCase,
	OldRimfireCase,
	PaintedHole,
	WallPlugHoles,
	Clock,
	/** Lead smear where a bullet skipped off a hard surface at a shallow angle. */
	RicochetMark,
	Pistol,
	Revolver
};

/** Whether a trace belongs to tonight's shooting. Hidden from the player; used only for scoring. */
UENUM(BlueprintType)
enum class EAMEvidenceRole : uint8
{
	Genuine,
	Decoy,
	Context
};

UENUM(BlueprintType)
enum class EAMTestResult : uint8
{
	NotTested,
	Negative,
	Weak,
	Positive
};

UENUM(BlueprintType)
enum class EAMPosture : uint8
{
	Standing,
	Kneeling
};

UENUM(BlueprintType)
enum class EAMProjectile : uint8
{
	None,
	Luger9mmFMJ,
	Special38LRN,
	/** .38 lead bullet flattened on one side by a ricochet, stone particles embedded. */
	Special38LRNDeformed
};

/** The player's call on what a defect is. Glass holes are called by the side the shot came from. */
UENUM(BlueprintType)
enum class EAMDefectCall : uint8
{
	Unclassified,
	Entry,
	Exit,
	Ricochet,
	NotBullet,
	FromNearSide,
	FromFarSide
};

/** How a firearm was found. Hidden from the player until they record it (visible parts) or make it safe (contents). */
USTRUCT(BlueprintType)
struct FAMWeaponState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString WeaponKey;
	/** Object path of the UAMFirearmData asset with the model, e.g. /Game/Aftermath/Firearms/DA_Firearm_Pistol9mm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString FirearmAsset;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Safety;
	/** Slide / hammer / cylinder position as found. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Action;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHasMagazine = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bMagazineSeated = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MagazineCapacity = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MagazineRounds = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString MagazineHeadstamp;
	/** Pistols: what is in the chamber. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ChamberState;
	/** Revolvers: each chamber, in the order CylinderNote describes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FString> CylinderChambers;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString CylinderNote;
};

USTRUCT(BlueprintType)
struct FAMShooter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Key;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Name;
	/** Muzzle position in world space, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Muzzle = FVector::ZeroVector;
	/** Short key matched against the player's weapon claims, e.g. "9mm", "38". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString WeaponKey;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Weapon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EAMPosture Posture = EAMPosture::Standing;
};

USTRUCT(BlueprintType)
struct FAMShot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Shooter;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Caliber;
	/** Firing order, 1 = first shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Order = 0;
	/** Unit direction of travel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Direction = FVector::ForwardVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FString> EvidenceIds;
};

/** Authored definition of one trace in the scene. */
USTRUCT(BlueprintType)
struct FAMEvidenceDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EAMEvidenceKind Kind = EAMEvidenceKind::DrywallEntry;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EAMEvidenceRole Role = EAMEvidenceRole::Genuine;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Location;
	/** Id in the surface library (e.g. "drywall_painted") of the material this trace is on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SurfaceId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Position = FVector::ZeroVector;
	/** Surface normal, pointing out of the surface toward the side the trace is seen from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector Normal = FVector::UpVector;
	/** Surface basis used for the long-axis angle: 0 deg = SurfaceRight, 90 deg = SurfaceUp. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector SurfaceRight = FVector::RightVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector SurfaceUp = FVector::UpVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ShotId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DiameterMm = 0.f;
	/** True angle between trajectory and surface. 90 = perpendicular. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ImpactAngleDeg = 90.f;
	/** True direction of the ellipse's long axis in the surface basis. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float LongAxisDeg = 0.f;
	/** Glass: names for the side the normal faces and the other side, e.g. "inside the kitchen" / "outside". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NearSideLabel;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString FarSideLabel;
	/** Ricochet: angle the bullet left the surface at, degrees above it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DepartureAngleDeg = 0.f;
	/** Firearms only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FAMWeaponState Weapon;
	/** Exit/entry partner for a through-and-through defect (rod placement). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PartnerId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EAMProjectile Projectile = EAMProjectile::None;
	/** Only visible to a crouched player with the flashlight on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHidden = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bOnFloor = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float YawDeg = 0.f;
	/** Seed for measurement noise, so readings are stable across sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Seed = 0;
};

USTRUCT(BlueprintType)
struct FAMTolerances
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ShooterFullCm = 75.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ShooterPartialCm = 150.f;
	/** Two strings closer than this are drawn as converging. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ConvergenceRadiusCm = 32.f;
};

/** A caliper reading, with realistic noise. */
USTRUCT(BlueprintType)
struct FAMMeasurement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	UPROPERTY(BlueprintReadOnly) float WidthMm = 0.f;
	UPROPERTY(BlueprintReadOnly) float LengthMm = 0.f;
	UPROPERTY(BlueprintReadOnly) float ImpactAngleDeg = 0.f;
	UPROPERTY(BlueprintReadOnly) float LongAxisDeg = 0.f;
	/** Set instead of numbers when the defect cannot be measured meaningfully. */
	UPROPERTY(BlueprintReadOnly) FString Note;
};

/** A trajectory string run by the player. */
USTRUCT(BlueprintType)
struct FAMTrajectoryString
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString EvidenceId;
	UPROPERTY(BlueprintReadOnly) float VerticalDeg = 0.f;
	UPROPERTY(BlueprintReadOnly) float HorizontalDeg = 0.f;
	UPROPERTY(BlueprintReadOnly) float ErrorDeg = 4.f;
	UPROPERTY(BlueprintReadOnly) FVector Origin = FVector::ZeroVector;
	/** Unit vector from the defect back toward the shooter. */
	UPROPERTY(BlueprintReadOnly) FVector BackDirection = FVector::ForwardVector;
	UPROPERTY(BlueprintReadOnly) float LengthCm = 0.f;
	UPROPERTY() TObjectPtr<class AActor> Visual = nullptr;
};

/** A shooter figure placed by the player. Muzzle = the floor point the player aimed at, at posture height. */
USTRUCT(BlueprintType)
struct FAMFigure
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Number = 0;
	UPROPERTY(BlueprintReadOnly) FVector FloorPoint = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) float YawDeg = 0.f;
	UPROPERTY(BlueprintReadWrite) EAMPosture Posture = EAMPosture::Standing;
	/** Shooter key the player assigned, empty = unidentified. */
	UPROPERTY(BlueprintReadWrite) FString Identity;
};

/** The player's conclusions. Strings use the same keys as the case data. */
USTRUCT(BlueprintType)
struct FAMClaims
{
	GENERATED_BODY()

	/** -1 = not stated. */
	UPROPERTY(BlueprintReadWrite) int32 ShotCount = -1;
	UPROPERTY(BlueprintReadWrite) TArray<FString> WeaponKeys;
	/** Shooter key, "undetermined", or empty. */
	UPROPERTY(BlueprintReadWrite) FString FirstShooter;
	/** "Consistent", "Contradicted", or empty. */
	UPROPERTY(BlueprintReadWrite) FString StatementVerdict;
};

/** What is written on an evidence tag when an item is bagged. */
USTRUCT(BlueprintType)
struct FAMEvidenceTag
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString CaseNumber;
	UPROPERTY(BlueprintReadOnly) int32 ItemNumber = 0;
	UPROPERTY(BlueprintReadOnly) FString Description;
	UPROPERTY(BlueprintReadOnly) FString Location;
	UPROPERTY(BlueprintReadOnly) FString DateTime;
	UPROPERTY(BlueprintReadOnly) FString CollectedBy;
};

USTRUCT(BlueprintType)
struct FAMScoreLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Label;
	UPROPERTY(BlueprintReadOnly) FString Detail;
	UPROPERTY(BlueprintReadOnly) int32 Points = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxPoints = 0;
};

USTRUCT(BlueprintType)
struct FAMScoreResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Total = 0;
	UPROPERTY(BlueprintReadOnly) FString Tier;
	UPROPERTY(BlueprintReadOnly) TArray<FAMScoreLine> Lines;
};
