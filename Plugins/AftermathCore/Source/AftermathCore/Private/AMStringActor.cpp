#include "AMStringActor.h"
#include "CableComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAMStringActor::AAMStringActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Cable = CreateDefaultSubobject<UCableComponent>(TEXT("Cable"));
	RootComponent = Cable;
	Cable->CableWidth = 0.35f;
	Cable->NumSides = 6;
	Cable->NumSegments = 24;
	Cable->SolverIterations = 8;
	Cable->bAttachStart = true;
	Cable->bAttachEnd = true;
	Cable->bEnableStiffness = false;
	Cable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cable->CastShadow = true;

	AnchorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Anchor"));
	AnchorMesh->SetupAttachment(RootComponent);
	AnchorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		AnchorMesh->SetStaticMesh(Cylinder.Object);
	}
	AnchorMesh->SetRelativeScale3D(FVector(0.024f, 0.024f, 0.03f));
}

void AAMStringActor::Setup(const FVector& Pin, const FVector& Anchor)
{
	SetActorLocation(Pin);
	SetActorRotation(FRotator::ZeroRotator);

	const FVector Delta = Anchor - Pin;
	Cable->EndLocation = Delta;
	// A little slack so the string sags like a real one over long runs.
	Cable->CableLength = Delta.Size() * 1.0025f;

	if (UMaterialInterface* Red = Cast<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/Aftermath/Materials/MI_String_Red.MI_String_Red")).TryLoad()))
	{
		Cable->SetMaterial(0, Red);
	}
	AnchorMesh->SetWorldLocation(Anchor);
	Cable->SetAttachEndTo(nullptr, NAME_None);
	Cable->MarkRenderStateDirty();
}
