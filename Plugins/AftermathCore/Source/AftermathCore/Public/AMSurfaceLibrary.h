// Aftermath — how each crime-scene surface material takes a bullet. One library for the whole game.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AMSurfaceLibrary.generated.h"

/** What a handgun bullet typically does to this surface at a normal angle. */
UENUM(BlueprintType)
enum class EAMSurfaceResponse : uint8
{
	PassesThrough,
	Stops,
	Shatters,
	Deflects
};

/** Gameplay profile of one surface material. Values are tunable design values, not laboratory data. */
USTRUCT(BlueprintType)
struct FAMSurfaceProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
	/** Wall, Floor, Furniture, Glass, Vehicle, Fabric, Exterior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Category;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EAMSurfaceResponse Response = EAMSurfaceResponse::PassesThrough;
	/** Below this impact angle (degrees to the surface) a bullet tends to skip instead of penetrate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RicochetCriticalAngleDeg = 10.f;
	/** Hole diameter relative to the bullet: below 1 = material closes up, above 1 = chips oversize. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HoleSizeFactor = 1.f;
	/** Whether a readable bullet wipe ring forms on entry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bShowsBulletWipe = true;
	/** Whether the projectile is usually recoverable from this material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHoldsProjectile = false;
	/** Texture names (in /Game/Aftermath/Textures) for each defect type; empty = not made yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString EntryTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ExitTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString RicochetTexture;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Notes;
};

USTRUCT()
struct FAMSurfaceLibraryJson
{
	GENERATED_BODY()

	UPROPERTY() TArray<FAMSurfaceProfile> Surfaces;
};

UCLASS(BlueprintType)
class AFTERMATHCORE_API UAMSurfaceLibrary : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces") TArray<FAMSurfaceProfile> Surfaces;
	UPROPERTY(EditAnywhere, Category = "Import") FFilePath JsonFile;

	UFUNCTION(CallInEditor, Category = "Import") void ReimportFromJson();
	UFUNCTION(BlueprintCallable, Category = "Import") bool ImportFromJsonFile(const FString& Path);

	const FAMSurfaceProfile* Find(const FString& Id) const;
};
