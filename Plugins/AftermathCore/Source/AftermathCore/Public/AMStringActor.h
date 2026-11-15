// Aftermath — a physical trajectory string (red cable with sag) between a pin and an anchor.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AMStringActor.generated.h"

class UCableComponent;
class UStaticMeshComponent;

UCLASS()
class AFTERMATHCORE_API AAMStringActor : public AActor
{
	GENERATED_BODY()

public:
	AAMStringActor();

	void Setup(const FVector& Pin, const FVector& Anchor);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCableComponent> Cable;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> AnchorMesh;
};
