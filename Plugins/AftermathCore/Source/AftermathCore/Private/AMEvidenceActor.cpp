#include "AMEvidenceActor.h"
#include "AMFirearmData.h"
#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace AMEvidence
{
	/** Material instances are found by naming convention; the Python setup script creates them. */
	static const TCHAR* DecalMaterialPath(EAMEvidenceKind Kind)
	{
		switch (Kind)
		{
		case EAMEvidenceKind::DrywallEntry:  return TEXT("/Game/Aftermath/Materials/MI_Defect_DrywallEntry.MI_Defect_DrywallEntry");
		case EAMEvidenceKind::DrywallExit:   return TEXT("/Game/Aftermath/Materials/MI_Defect_DrywallExit.MI_Defect_DrywallExit");
		case EAMEvidenceKind::PlasterCrater: return TEXT("/Game/Aftermath/Materials/MI_Defect_PlasterCrater.MI_Defect_PlasterCrater");
		case EAMEvidenceKind::SteelHole:     return TEXT("/Game/Aftermath/Materials/MI_Defect_SteelHole.MI_Defect_SteelHole");
		case EAMEvidenceKind::PaintedHole:   return TEXT("/Game/Aftermath/Materials/MI_Decoy_PaintedHole.MI_Decoy_PaintedHole");
		case EAMEvidenceKind::WallPlugHoles: return TEXT("/Game/Aftermath/Materials/MI_Decoy_WallPlugs.MI_Decoy_WallPlugs");
		case EAMEvidenceKind::RicochetMark:  return TEXT("/Game/Aftermath/Materials/MI_Defect_Ricochet.MI_Defect_Ricochet");
		default: return nullptr;
		}
	}
}

AAMEvidenceActor::AAMEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	PickVolume = CreateDefaultSubobject<USphereComponent>(TEXT("PickVolume"));
	PickVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PickVolume->SetGenerateOverlapEvents(false);
	PickVolume->SetHiddenInGame(true);
	RootComponent = PickVolume;

	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	Decal->SetupAttachment(RootComponent);
	Decal->SetVisibility(false);

	Prop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Prop"));
	Prop->SetupAttachment(RootComponent);
	Prop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Prop->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Prop->SetStaticMesh(Cylinder.Object);
	}

	Part = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Part"));
	Part->SetupAttachment(Prop);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetVisibility(false);

	Marker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Marker->SetHorizontalAlignment(EHTA_Center);
	Marker->SetVerticalAlignment(EVRTA_TextCenter);
	Marker->SetTextRenderColor(FColor(240, 190, 46));
	Marker->SetWorldSize(4.f);
	Marker->SetVisibility(false);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

float AAMEvidenceActor::FieldCm(EAMEvidenceKind Kind)
{
	switch (Kind)
	{
	case EAMEvidenceKind::DrywallEntry:  return 4.0f;
	case EAMEvidenceKind::DrywallExit:   return 9.6f;
	case EAMEvidenceKind::PlasterCrater: return 6.4f;
	case EAMEvidenceKind::SteelHole:     return 3.6f;
	case EAMEvidenceKind::PaintedHole:   return 4.0f;
	case EAMEvidenceKind::WallPlugHoles: return 12.0f;
	case EAMEvidenceKind::GlassHole:     return 38.0f;
	case EAMEvidenceKind::Clock:         return 38.0f;
	case EAMEvidenceKind::RicochetMark:  return 12.0f;
	case EAMEvidenceKind::Pistol:
	case EAMEvidenceKind::Revolver:      return 30.0f;
	default:                             return 3.4f;
	}
}

float AAMEvidenceActor::PickRadiusCm(EAMEvidenceKind Kind)
{
	switch (Kind)
	{
	case EAMEvidenceKind::Clock:         return 17.f;
	case EAMEvidenceKind::Pistol:
	case EAMEvidenceKind::Revolver:      return 14.f;
	case EAMEvidenceKind::RicochetMark:  return 6.f;
	case EAMEvidenceKind::WallPlugHoles: return 7.f;
	case EAMEvidenceKind::GlassHole:     return 5.f;
	default:                             return 4.5f;
	}
}

void AAMEvidenceActor::InitFromDef(const FAMEvidenceDef& InDef)
{
	Def = InDef;
	SetActorLocation(Def.Position);
	PickVolume->SetSphereRadius(PickRadiusCm(Def.Kind));

	if (const TCHAR* Path = AMEvidence::DecalMaterialPath(Def.Kind))
	{
		if (UMaterialInterface* Mat = Cast<UMaterialInterface>(FSoftObjectPath(Path).TryLoad()))
		{
			const float Half = FieldCm(Def.Kind) * 0.5f;
			Decal->SetDecalMaterial(Mat);
			Decal->DecalSize = FVector(3.f, Half, Half);
			// Decal projects along its X axis, into the surface. Roll turns the defect's long axis to the true direction.
			const FRotator Base = FRotationMatrix::MakeFromXZ(-Def.Normal, Def.SurfaceUp).Rotator();
			Decal->SetWorldRotation(Base);
			Decal->AddLocalRotation(FRotator(0.f, 0.f, -Def.LongAxisDeg));
			Decal->SetVisibility(true);
		}
	}

	if (Def.Kind == EAMEvidenceKind::CartridgeCase || Def.Kind == EAMEvidenceKind::OldRimfireCase)
	{
		const bool bRimfire = Def.Kind == EAMEvidenceKind::OldRimfireCase;
		const float Dia = bRimfire ? 0.57f : 0.965f;
		const float Len = bRimfire ? 1.56f : 1.915f;
		Prop->SetWorldScale3D(FVector(Dia / 100.f, Dia / 100.f, Len / 100.f));
		Prop->SetWorldRotation(FRotator(90.f, Def.YawDeg, 0.f));
		Prop->SetWorldLocation(FVector(Def.Position.X, Def.Position.Y, Dia * 0.5f));
		if (UMaterialInterface* Brass = Cast<UMaterialInterface>(FSoftObjectPath(
			bRimfire ? TEXT("/Game/Aftermath/Materials/MI_Brass_Tarnished.MI_Brass_Tarnished")
			         : TEXT("/Game/Aftermath/Materials/MI_Brass.MI_Brass")).TryLoad()))
		{
			Prop->SetMaterial(0, Brass);
		}
		Prop->SetVisibility(true);
	}
	else if (IsWeapon())
	{
		const bool bRevolver = Def.Kind == EAMEvidenceKind::Revolver;
		const UAMFirearmData* Firearm = Def.Weapon.FirearmAsset.IsEmpty() ? nullptr
			: Cast<UAMFirearmData>(FSoftObjectPath(Def.Weapon.FirearmAsset).TryLoad());
		UStaticMesh* Body = Firearm ? Firearm->BodyMesh.LoadSynchronous() : nullptr;
		if (Body)
		{
			// Real model: lies on its side at the case position, parts attached at their sockets.
			Prop->SetStaticMesh(Body);
			Prop->SetWorldScale3D(FVector::OneVector);
			Prop->SetWorldRotation(FRotator(0.f, Def.YawDeg, 0.f) + Firearm->RestRotation);
			Prop->SetWorldLocation(Def.Position);
			Prop->SetVisibility(true);
			UStaticMesh* PartMesh = bRevolver ? Firearm->CylinderMesh.LoadSynchronous()
				: (Def.Weapon.bMagazineSeated ? Firearm->MagazineMesh.LoadSynchronous() : nullptr);
			if (PartMesh)
			{
				const FName Socket = bRevolver ? Firearm->CylinderSocket : Firearm->MagazineSocket;
				Part->SetStaticMesh(PartMesh);
				Part->AttachToComponent(Prop, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
					Prop->DoesSocketExist(Socket) ? Socket : NAME_None);
				Part->SetVisibility(true);
			}
			return;
		}

		// No model assigned yet: a dark block the size of the gun, lying flat.
		const FVector SizeCm = Firearm ? Firearm->PlaceholderSizeCm : (bRevolver ? FVector(24.f, 14.f, 4.f) : FVector(19.f, 13.f, 3.f));
		Prop->SetStaticMesh(Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad()));
		Prop->SetWorldScale3D(SizeCm / 100.f);
		Prop->SetWorldRotation(FRotator(0.f, Def.YawDeg, 0.f));
		Prop->SetWorldLocation(FVector(Def.Position.X, Def.Position.Y, Def.Position.Z + SizeCm.Z * 0.5f - 1.5f));
		if (UMaterialInterface* Gunmetal = Cast<UMaterialInterface>(FSoftObjectPath(
			TEXT("/Game/Aftermath/Materials/MI_Gunmetal.MI_Gunmetal")).TryLoad()))
		{
			Prop->SetMaterial(0, Gunmetal);
		}
		Prop->SetVisibility(true);
	}
	else if (Def.Kind == EAMEvidenceKind::Clock)
	{
		Prop->SetWorldScale3D(FVector(0.32f, 0.32f, 0.035f));
		Prop->SetWorldLocation(FVector(Def.Position.X, Def.Position.Y, 1.75f));
		Prop->SetVisibility(true);
	}
}

bool AAMEvidenceActor::IsSurfaceDefect() const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::DrywallEntry:
	case EAMEvidenceKind::DrywallExit:
	case EAMEvidenceKind::PlasterCrater:
	case EAMEvidenceKind::SteelHole:
	case EAMEvidenceKind::GlassHole:
	case EAMEvidenceKind::PaintedHole:
	case EAMEvidenceKind::WallPlugHoles:
	case EAMEvidenceKind::RicochetMark:
		return true;
	default:
		return false;
	}
}

bool AAMEvidenceActor::IsWeapon() const
{
	return Def.Kind == EAMEvidenceKind::Pistol || Def.Kind == EAMEvidenceKind::Revolver;
}

bool AAMEvidenceActor::IsCallable() const
{
	return IsSurfaceDefect();
}

TArray<EAMDefectCall> AAMEvidenceActor::CallOptions() const
{
	if (Def.Kind == EAMEvidenceKind::GlassHole)
	{
		return {EAMDefectCall::FromNearSide, EAMDefectCall::FromFarSide, EAMDefectCall::Unclassified};
	}
	return {EAMDefectCall::Entry, EAMDefectCall::Exit, EAMDefectCall::Ricochet, EAMDefectCall::NotBullet, EAMDefectCall::Unclassified};
}

EAMDefectCall AAMEvidenceActor::TruthCall(const FVector& ShotDirection) const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::DrywallEntry:
	case EAMEvidenceKind::SteelHole:
	case EAMEvidenceKind::PlasterCrater:  return EAMDefectCall::Entry;   // the bullet went in and stayed
	case EAMEvidenceKind::DrywallExit:    return EAMDefectCall::Exit;
	case EAMEvidenceKind::RicochetMark:   return EAMDefectCall::Ricochet;
	case EAMEvidenceKind::PaintedHole:
	case EAMEvidenceKind::WallPlugHoles:  return EAMDefectCall::NotBullet;
	case EAMEvidenceKind::GlassHole:
		// Normal faces the near side. Travelling against the normal means the shot came from the near side.
		return FVector::DotProduct(ShotDirection, Def.Normal) < 0.f ? EAMDefectCall::FromNearSide : EAMDefectCall::FromFarSide;
	default:                              return EAMDefectCall::Unclassified;
	}
}

FString AAMEvidenceActor::CallLabel(EAMDefectCall InCall) const
{
	switch (InCall)
	{
	case EAMDefectCall::Entry:        return TEXT("Entry");
	case EAMDefectCall::Exit:         return TEXT("Exit");
	case EAMDefectCall::Ricochet:     return TEXT("Ricochet");
	case EAMDefectCall::NotBullet:    return TEXT("Not a bullet defect");
	case EAMDefectCall::FromNearSide: return FString::Printf(TEXT("Fired from %s"), Def.NearSideLabel.IsEmpty() ? TEXT("this side") : *Def.NearSideLabel);
	case EAMDefectCall::FromFarSide:  return FString::Printf(TEXT("Fired from %s"), Def.FarSideLabel.IsEmpty() ? TEXT("the far side") : *Def.FarSideLabel);
	default:                          return TEXT("Not called");
	}
}

FString AAMEvidenceActor::GetEdgeFinding(const FVector& ShotDirection) const
{
	// The exit face of glass loses a cone of flakes, so the crater (and the bevel on the hole's edge) widens toward it.
	const bool bFromNear = FVector::DotProduct(ShotDirection, Def.Normal) < 0.f;
	const FString ExitFace = bFromNear ? Def.FarSideLabel : Def.NearSideLabel;
	const FString EntryFace = bFromNear ? Def.NearSideLabel : Def.FarSideLabel;
	FString Finding = FString::Printf(TEXT("Small, sharp-edged opening on the face toward %s. On the face toward %s the hole opens into a "
		"shallow cone: flakes are missing and the edge is bevelled."), *EntryFace, *ExitFace);

	// Side-to-side angle from the exit crater: little chipping on the side the bullet came from, heavy chipping opposite;
	// at the entry, short straight radial cracks on that side, one or two long ones opposite.
	const FVector EntryNormal = bFromNear ? Def.Normal : -Def.Normal;          // points toward the entry side
	const FVector Facing = (-EntryNormal * FVector(1.f, 1.f, 0.f)).GetSafeNormal(); // viewer on the entry side, facing the glass
	const FVector ViewerRight(-Facing.Y, Facing.X, 0.f);
	const float Sideways = FVector::DotProduct(ShotDirection.GetSafeNormal(), ViewerRight);
	const float SideAngleDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Abs(Sideways)));
	if (SideAngleDeg < 5.f)
	{
		Finding += TEXT(" Chipping around the exit crater is even all round: the bullet struck almost straight on, side to side.");
	}
	else
	{
		// Moving toward the viewer's right means it came in from the left, and vice versa.
		const TCHAR* From = Sideways > 0.f ? TEXT("left") : TEXT("right");
		const TCHAR* Other = Sideways > 0.f ? TEXT("right") : TEXT("left");
		Finding += FString::Printf(TEXT(" Seen from %s, the exit crater is barely chipped on the %s and heavily chipped on the %s; "
			"at the entry, the radial cracks are short and straight on the %s and long on the %s. The bullet came in from the %s, "
			"%s."), *EntryFace, From, Other, From, Other, From,
			SideAngleDeg > 30.f ? TEXT("at a steep sideways angle") : TEXT("at a moderate sideways angle"));
	}
	Finding += TEXT(" Where this hole's cracks run into the other hole's cracks, they stop: whichever crack stops at the other was made later.");
	return Finding;
}

TArray<FString> AAMEvidenceActor::GetConditionLines() const
{
	const FAMWeaponState& W = Def.Weapon;
	TArray<FString> L;
	L.Add(W.Description);
	L.Add(TEXT("Action: ") + W.Action);
	L.Add(TEXT("Safety: ") + W.Safety);
	if (Def.Kind == EAMEvidenceKind::Revolver)
	{
		L.Add(TEXT("Arrow scratched on the cylinder at the chamber under the firing pin; cylinder diagram started."));
	}
	if (W.bHasMagazine)
	{
		L.Add(W.bMagazineSeated ? TEXT("Magazine: seated in the grip") : TEXT("Magazine: not inserted"));
	}
	return L;
}

TArray<FString> AAMEvidenceActor::GetUnloadLines() const
{
	const FAMWeaponState& W = Def.Weapon;
	TArray<FString> L;
	if (Def.Kind == EAMEvidenceKind::Revolver && bConditionLost)
	{
		// Without the arrow at the chamber under the firing pin, the diagram can't relate chambers to the firing order.
		int32 Fired = 0;
		for (const FString& C : W.CylinderChambers) { if (C.Contains(TEXT("Fired"))) { ++Fired; } }
		L.Add(FString::Printf(TEXT("Cylinder opened: %d fired cases, %d live rounds."), Fired, W.CylinderChambers.Num() - Fired));
		L.Add(TEXT("The chamber under the firing pin was not marked first, so the order of the chambers is lost."));
	}
	else if (Def.Kind == EAMEvidenceKind::Revolver)
	{
		L.Add(TEXT("Cylinder opened. ") + W.CylinderNote);
		for (int32 i = 0; i < W.CylinderChambers.Num(); ++i)
		{
			L.Add(FString::Printf(TEXT("  Chamber %d: %s"), i + 1, *W.CylinderChambers[i]));
		}
	}
	else
	{
		if (W.bHasMagazine)
		{
			L.Add(FString::Printf(TEXT("Magazine removed: %d of %d rounds, headstamp %s"), W.MagazineRounds, W.MagazineCapacity, *W.MagazineHeadstamp));
		}
		L.Add(TEXT("Slide locked back. Chamber: ") + (W.ChamberState.IsEmpty() ? FString(TEXT("empty")) : W.ChamberState));
	}
	return L;
}

bool AAMEvidenceActor::CanTakeString() const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::DrywallEntry: return Measurement.bValid || bRodPlaced;
	case EAMEvidenceKind::GlassHole:
	case EAMEvidenceKind::SteelHole:    return Measurement.bValid;
	case EAMEvidenceKind::RicochetMark: return Measurement.bValid;
	default:                            return false;
	}
}

FString AAMEvidenceActor::GetShortLabel() const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::CartridgeCase:
	case EAMEvidenceKind::OldRimfireCase: return TEXT("Small brass object");
	case EAMEvidenceKind::GlassHole:      return TEXT("Glass damage");
	case EAMEvidenceKind::Clock:          return TEXT("Wall clock");
	case EAMEvidenceKind::Pistol:         return TEXT("Pistol");
	case EAMEvidenceKind::Revolver:       return TEXT("Revolver");
	case EAMEvidenceKind::RicochetMark:   return TEXT("Grey smear");
	default:                              return TEXT("Surface defect");
	}
}

FString AAMEvidenceActor::GetMaterialLabel() const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::DrywallEntry:
	case EAMEvidenceKind::DrywallExit:   return TEXT("Painted drywall");
	case EAMEvidenceKind::PlasterCrater: return TEXT("Plaster on masonry");
	case EAMEvidenceKind::SteelHole:     return TEXT("Stainless steel panel");
	case EAMEvidenceKind::GlassHole:     return TEXT("Float glass");
	case EAMEvidenceKind::PaintedHole:
	case EAMEvidenceKind::WallPlugHoles: return TEXT("Painted plaster");
	case EAMEvidenceKind::RicochetMark:  return TEXT("Polished stone worktop");
	case EAMEvidenceKind::Pistol:
	case EAMEvidenceKind::Revolver:      return TEXT("Firearm");
	default:                             return TEXT("Floor find");
	}
}

FString AAMEvidenceActor::GetObservation() const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::DrywallEntry:
		return TEXT("Round-to-oval hole in painted drywall. The paper edge is pushed inward and a dark grey ring runs around the margin, smeared toward one side.");
	case EAMEvidenceKind::DrywallExit:
		return TEXT("Ragged opening with torn paper flaps and crumbled gypsum, all pushed outward toward you. No ring at the margin.");
	case EAMEvidenceKind::PlasterCrater:
		return TEXT("Shallow crater with chipped edges in the plaster. A grey metallic object sits at the base.");
	case EAMEvidenceKind::SteelHole:
		return TEXT("Clean hole in the door panel with a bright scraped ring and a grey smear trailing off one side. Something rattles behind the panel.");
	case EAMEvidenceKind::GlassHole:
		return TEXT("Small hole with radial cracks running out and short concentric cracks around it. Crack lines from this hole meet those of the other hole.");
	case EAMEvidenceKind::PaintedHole:
		return TEXT("Shallow round depression. The paint film runs unbroken across it and dust has settled in the recess.");
	case EAMEvidenceKind::WallPlugHoles:
		return TEXT("Two small holes 80 mm apart at the same height, with remains of grey plastic wall plugs.");
	case EAMEvidenceKind::CartridgeCase:
		return TEXT("Bright brass cartridge case, no tarnish. Headstamp 9mm LUGER. Firing-pin strike in the primer.");
	case EAMEvidenceKind::OldRimfireCase:
		return TEXT("Small rimfire case, dull and heavily tarnished. A cobweb strand runs across it. Headstamp: a raised H.");
	case EAMEvidenceKind::Clock:
		return TEXT("Wall clock face up on the floor, glass cracked. Hands stopped at 2:14. An empty nail sits on the wall above.");
	case EAMEvidenceKind::RicochetMark:
		return TEXT("Long, narrow grey smear on the stone, about the width of a bullet. It starts as a faint graze and widens; at the wide end the "
			"stone is chipped and tiny grey flecks are splashed beyond it. No hole: the surface is too hard to penetrate.");
	case EAMEvidenceKind::Pistol:
		return TEXT("Black semi-automatic pistol lying on its side on the worktop. Do not touch it until its condition is recorded.");
	case EAMEvidenceKind::Revolver:
		return TEXT("Revolver lying on the floor by the front door, muzzle toward the wall. Do not touch it until its condition is recorded.");
	default:
		return FString();
	}
}

FString AAMEvidenceActor::GetProjectileDescription() const
{
	switch (Def.Projectile)
	{
	case EAMProjectile::Luger9mmFMJ:
		return TEXT("9 mm Luger, full metal jacket (copper), 8.0 g, nose flattened. Six lands and grooves, right twist.");
	case EAMProjectile::Special38LRN:
		return TEXT(".38 Special, plain lead round-nose, no jacket, 10.2 g. Five lands and grooves, right twist.");
	case EAMProjectile::Special38LRNDeformed:
		return TEXT(".38 Special plain lead bullet, 10.1 g, flattened along one side with grey stone particles pressed into the flat. "
			"It struck something hard before it stopped here. Five lands and grooves, right twist.");
	default:
		return FString();
	}
}

EAMTestResult AAMEvidenceActor::EvaluateLead() const
{
	if (Def.Role != EAMEvidenceRole::Genuine || !IsSurfaceDefect())
	{
		return EAMTestResult::Negative;
	}
	return Def.Kind == EAMEvidenceKind::DrywallExit ? EAMTestResult::Weak : EAMTestResult::Positive;
}

EAMTestResult AAMEvidenceActor::EvaluateCopper(bool bShotWasJacketed) const
{
	if (Def.Role != EAMEvidenceRole::Genuine || !IsSurfaceDefect() || !bShotWasJacketed)
	{
		return EAMTestResult::Negative;
	}
	return Def.Kind == EAMEvidenceKind::DrywallExit ? EAMTestResult::Weak : EAMTestResult::Positive;
}

void AAMEvidenceActor::SetMarker(int32 Number, const FVector& ViewerLocation)
{
	MarkerNumber = Number;
	Marker->SetText(FText::AsNumber(Number));

	if (Def.bOnFloor)
	{
		FVector ToViewer = ViewerLocation - Def.Position;
		ToViewer.Z = 0.f;
		ToViewer = ToViewer.GetSafeNormal();
		Marker->SetWorldLocation(FVector(Def.Position.X, Def.Position.Y, 5.f) + ToViewer * 8.f);
		Marker->SetWorldRotation(ToViewer.Rotation());
	}
	else
	{
		const float Offset = FieldCm(Def.Kind) * 0.6f + 2.f;
		Marker->SetWorldLocation(Def.Position + Def.SurfaceRight * Offset + Def.SurfaceUp * Offset * 0.8f + Def.Normal * 0.5f);
		Marker->SetWorldRotation(Def.Normal.Rotation());
	}
	Marker->SetVisibility(true);
}

void AAMEvidenceActor::SetExcluded(bool bInExcluded)
{
	bExcluded = bInExcluded;
	Marker->SetVisibility(IsDocumented() && !bExcluded);
}

void AAMEvidenceActor::ShowMadeSafe()
{
	if (!IsWeapon() || !Part->IsVisible())
	{
		return;
	}
	if (Def.Kind == EAMEvidenceKind::Revolver)
	{
		// Cylinder swung out to the left of the frame.
		Part->AddLocalOffset(FVector(0.f, -3.5f, 0.f));
		Part->AddLocalRotation(FRotator(0.f, 0.f, -30.f));
	}
	else
	{
		// Magazine out, laid beside the pistol.
		Part->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		Part->SetWorldLocation(Def.Position + Def.SurfaceUp * 14.f);
	}
}

bool AAMEvidenceActor::IsCollectable() const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::CartridgeCase:
	case EAMEvidenceKind::OldRimfireCase:
	case EAMEvidenceKind::Pistol:
	case EAMEvidenceKind::Revolver:
	case EAMEvidenceKind::Clock:
		return true;
	default:
		// A recovered projectile is collected; the wall or panel it came from stays.
		return Def.Projectile != EAMProjectile::None && bProjectileRecovered;
	}
}

FString AAMEvidenceActor::GetPackagingNote() const
{
	switch (Def.Kind)
	{
	case EAMEvidenceKind::CartridgeCase:
	case EAMEvidenceKind::OldRimfireCase:
		return TEXT("Case placed in a padded container and sealed with paper tape. The container is marked across the tape; the case itself is never marked.");
	case EAMEvidenceKind::Pistol:
		return TEXT("Pistol tagged and marked on the right side of the frame, on the slide and on the barrel; magazine marked on its base. Wrapped clean, not cleaned.");
	case EAMEvidenceKind::Revolver:
		return TEXT("Revolver tagged and marked on the frame, cylinder and barrel; cylinder diagram attached. Wrapped clean, not cleaned.");
	case EAMEvidenceKind::Clock:
		return TEXT("Clock bagged as found.");
	default:
		return TEXT("Bullet placed in a cotton-padded vial and sealed with paper tape. The vial is marked; the bullet itself is never marked.");
	}
}
