// Aftermath — one firearm model: its meshes, sockets and facts. Case data points at these assets by path.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AMFirearmData.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EAMFirearmAction : uint8
{
	SemiAutoPistol,
	Revolver,
	Rifle,
	Shotgun
};

/**
 * Real-world firearm models go in as static meshes split into parts (body, magazine, cylinder).
 * Leave BodyMesh empty and the game shows a grey placeholder block of PlaceholderSizeCm.
 * DisplayName stays generic ("9 mm striker-fired pistol"): no brand names or logos in game text or textures.
 */
UCLASS(BlueprintType)
class AFTERMATHCORE_API UAMFirearmData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") FText DisplayName;
	/** Matches the case's weapon keys and the player's weapon claims, e.g. "9mm", "38". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") FString WeaponKey;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") FString Caliber;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") EAMFirearmAction Action = EAMFirearmAction::SemiAutoPistol;
	/** Magazine or cylinder capacity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firearm") int32 Capacity = 15;

	/** Main mesh, real-world scale in cm, muzzle along +X, origin at the grip. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Model") TSoftObjectPtr<UStaticMesh> BodyMesh;
	/** Pistols: the magazine as its own mesh, attached at MagazineSocket while seated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Model") TSoftObjectPtr<UStaticMesh> MagazineMesh;
	/** Revolvers: the cylinder as its own mesh, attached at CylinderSocket while closed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Model") TSoftObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Model") FName MagazineSocket = TEXT("Magazine");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Model") FName CylinderSocket = TEXT("Cylinder");
	/** Rotation that lays the model on its side on a surface (added to the case's yaw). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Model") FRotator RestRotation = FRotator(0.f, 0.f, 90.f);
	/** Size of the grey block shown while no model is assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Model") FVector PlaceholderSizeCm = FVector(19.f, 13.f, 3.f);
};
