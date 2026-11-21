#include "AMCaseSubsystem.h"
#include "AMCaseData.h"
#include "AMEvidenceActor.h"
#include "AMStringActor.h"
#include "AMBallistics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"

DEFINE_LOG_CATEGORY_STATIC(LogAMCaseRun, Log, All);

TStatId UAMCaseSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAMCaseSubsystem, STATGROUP_Tickables);
}

void UAMCaseSubsystem::Say(const FString& Message)
{
	UE_LOG(LogAMCaseRun, Log, TEXT("%s"), *Message);
	OnMessage.Broadcast(Message);
}

void UAMCaseSubsystem::Changed()
{
	OnStateChanged.Broadcast();
}

void UAMCaseSubsystem::StartCase(UAMCaseData* InCase)
{
	if (!InCase)
	{
		UE_LOG(LogAMCaseRun, Error, TEXT("StartCase: no case asset."));
		return;
	}
	for (AAMEvidenceActor* E : Evidence) { if (E) { E->Destroy(); } }
	for (FAMTrajectoryString& S : Strings) { if (S.Visual) { S.Visual->Destroy(); } }
	Evidence.Reset();
	Strings.Reset();
	Figures.Reset();
	Claims = FAMClaims();
	NextMarker = 1;
	NextFigure = 1;
	bSubmitted = false;
	bShowTruth = false;

	Case = InCase;
	SwabsLeft = Case->Swabs;
	LabRequestsLeft = Case->LabRequests;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FAMEvidenceDef& Def : Case->Evidence)
	{
		AAMEvidenceActor* E = GetWorld()->SpawnActor<AAMEvidenceActor>(AAMEvidenceActor::StaticClass(), FTransform(Def.Position), Params);
		if (E)
		{
			E->InitFromDef(Def);
#if WITH_EDITOR
			E->SetActorLabel(FString::Printf(TEXT("Evidence_%s"), *Def.Id));
#endif
			Evidence.Add(E);
		}
	}
	Say(FString::Printf(TEXT("Case %s loaded: %d traces in the scene."), *Case->CaseId, Evidence.Num()));
	Changed();
}

int32 UAMCaseSubsystem::CountDocumented() const
{
	int32 N = 0;
	for (const AAMEvidenceActor* E : Evidence) { if (E && E->IsDocumented()) { ++N; } }
	return N;
}

AAMEvidenceActor* UAMCaseSubsystem::FindEvidenceActor(const FString& Id) const
{
	for (AAMEvidenceActor* E : Evidence) { if (E && E->Def.Id == Id) { return E; } }
	return nullptr;
}

bool UAMCaseSubsystem::IsJacketed(const FAMEvidenceDef& Def) const
{
	// Jacketed if the shot that made it came from a pistol firing FMJ. Kept in data, not hard-coded per case.
	if (const FAMShot* Shot = Case ? Case->FindShot(Def.ShotId) : nullptr)
	{
		return Shot->Caliber.Contains(TEXT("9mm")) || Shot->Caliber.Contains(TEXT("FMJ"));
	}
	return false;
}

bool UAMCaseSubsystem::Document(AAMEvidenceActor* E, FVector ViewerLocation)
{
	if (!E) { return false; }
	if (E->IsDocumented())
	{
		Say(FString::Printf(TEXT("Already marked as #%d."), E->MarkerNumber));
		return false;
	}
	E->SetMarker(NextMarker++, ViewerLocation);
	Say(FString::Printf(TEXT("Marked as #%d and photographed."), E->MarkerNumber));
	Changed();
	return true;
}

bool UAMCaseSubsystem::ToggleExcluded(AAMEvidenceActor* E)
{
	if (!E || !E->IsDocumented()) { Say(TEXT("Mark it first.")); return false; }
	E->SetExcluded(!E->bExcluded);
	Say(E->bExcluded ? FString::Printf(TEXT("#%d left out of the report."), E->MarkerNumber)
	                 : FString::Printf(TEXT("#%d back in the report."), E->MarkerNumber));
	Changed();
	return true;
}

bool UAMCaseSubsystem::Measure(AAMEvidenceActor* E)
{
	if (!E || !E->IsSurfaceDefect()) { Say(TEXT("Nothing to measure with calipers.")); return false; }
	if (E->Measurement.bValid || !E->Measurement.Note.IsEmpty()) { Say(TEXT("Already measured.")); return false; }
	E->Measurement = UAMBallistics::MeasureDefect(E->Def);
	if (E->Measurement.bValid)
	{
		Say(FString::Printf(TEXT("Calipers: %.1f x %.1f mm, long axis %.0f deg."),
			E->Measurement.WidthMm, E->Measurement.LengthMm, FMath::Fmod(E->Measurement.LongAxisDeg + 360.f, 360.f)));
	}
	else
	{
		Say(FString::Printf(TEXT("Calipers: %s"), *E->Measurement.Note));
	}
	Changed();
	return true;
}

bool UAMCaseSubsystem::TestLead(AAMEvidenceActor* E)
{
	if (!E || !E->IsSurfaceDefect()) { Say(TEXT("The lead test is for surface defects.")); return false; }
	if (E->LeadResult != EAMTestResult::NotTested) { Say(TEXT("Already tested for lead.")); return false; }
	if (SwabsLeft <= 0) { Say(TEXT("No swabs left.")); return false; }
	--SwabsLeft;
	E->LeadResult = E->EvaluateLead();
	Say(E->LeadResult == EAMTestResult::Positive ? TEXT("Rhodizonate: pink, turning blue-violet with acid.")
		: E->LeadResult == EAMTestResult::Weak ? TEXT("Rhodizonate: faint pink.") : TEXT("Rhodizonate: no colour change."));
	Changed();
	return true;
}

bool UAMCaseSubsystem::TestCopper(AAMEvidenceActor* E)
{
	if (!E || !E->IsSurfaceDefect()) { Say(TEXT("The copper test is for surface defects.")); return false; }
	if (E->CopperResult != EAMTestResult::NotTested) { Say(TEXT("Already tested for copper.")); return false; }
	if (SwabsLeft <= 0) { Say(TEXT("No swabs left.")); return false; }
	--SwabsLeft;
	E->CopperResult = E->EvaluateCopper(IsJacketed(E->Def));
	Say(E->CopperResult == EAMTestResult::Positive ? TEXT("DTO: dark green.")
		: E->CopperResult == EAMTestResult::Weak ? TEXT("DTO: faint olive tint.") : TEXT("DTO: no colour change."));
	Changed();
	return true;
}

bool UAMCaseSubsystem::PlaceRod(AAMEvidenceActor* E)
{
	if (!E || E->Def.PartnerId.IsEmpty()) { Say(TEXT("A rod needs a hole that goes all the way through.")); return false; }
	AAMEvidenceActor* Entry = (E->Def.Kind == EAMEvidenceKind::DrywallEntry) ? E : FindEvidenceActor(E->Def.PartnerId);
	AAMEvidenceActor* Exit = Entry ? FindEvidenceActor(Entry->Def.PartnerId) : nullptr;
	if (!Entry || !Exit) { return false; }
	if (Entry->bRodPlaced) { Say(TEXT("A rod is already in place.")); return false; }

	// True line through both faces, plus angle-finder error (~0.4 deg).
	FRandomStream Stream(Entry->Def.Seed + 11);
	FVector Back = (Entry->Def.Position - Exit->Def.Position).GetSafeNormal();
	Back = Back.RotateAngleAxis(UAMBallistics::Gaussian(Stream) * 0.4f, FVector::UpVector);
	Back.Z += UAMBallistics::Gaussian(Stream) * 0.006f;
	Back.Normalize();
	Entry->bRodPlaced = Exit->bRodPlaced = true;
	Entry->RodBackDirection = Exit->RodBackDirection = Back;

	float V = 0.f, H = 0.f;
	UAMBallistics::AnglesFromBackDirection(Entry->Def.Normal, Entry->Def.SurfaceRight, Entry->Def.SurfaceUp, Back, V, H);
	Say(FString::Printf(TEXT("Rod seated through both faces. Angle finder: vertical %+.1f deg, horizontal %+.1f deg."), V, H));
	Changed();
	return true;
}

bool UAMCaseSubsystem::RecoverProjectile(AAMEvidenceActor* E)
{
	if (!E || E->Def.Projectile == EAMProjectile::None) { Say(TEXT("No projectile here.")); return false; }
	if (E->bProjectileRecovered) { Say(TEXT("Already recovered.")); return false; }
	E->bProjectileRecovered = true;
	Say(FString::Printf(TEXT("Section cut out around the bullet and the bullet freed without metal tools, so its rifling marks survive: %s"),
		*E->GetProjectileDescription()));
	Changed();
	return true;
}

FAMTrajectoryString* UAMCaseSubsystem::FindString(const FString& EvidenceId)
{
	return Strings.FindByPredicate([&](const FAMTrajectoryString& S) { return S.EvidenceId == EvidenceId; });
}

bool UAMCaseSubsystem::RunString(AAMEvidenceActor* E)
{
	if (!E) { return false; }
	if (E->Def.Kind == EAMEvidenceKind::DrywallExit) { Say(TEXT("Strings run from the entry side. Use the kitchen face.")); return false; }
	if (!E->CanTakeString()) { Say(TEXT("Measure the defect (or seat a rod) before running a string.")); return false; }

	FVector Back;
	if (E->bRodPlaced)
	{
		Back = E->RodBackDirection;
	}
	else
	{
		Back = UAMBallistics::BackDirectionFromEllipse(E->Def.Normal, E->Def.SurfaceRight, E->Def.SurfaceUp,
			E->Measurement.ImpactAngleDeg, E->Measurement.LongAxisDeg);
	}
	float V = 0.f, H = 0.f;
	UAMBallistics::AnglesFromBackDirection(E->Def.Normal, E->Def.SurfaceRight, E->Def.SurfaceUp, Back, V, H);
	if (!bAssisted)
	{
		// Expert mode: the player dials the angles; start flat so nothing is given away.
		V = 0.f;
		H = 0.f;
	}

	FAMTrajectoryString* S = FindString(E->Def.Id);
	if (!S)
	{
		S = &Strings.AddDefaulted_GetRef();
		S->EvidenceId = E->Def.Id;
	}
	S->VerticalDeg = V;
	S->HorizontalDeg = H;
	S->ErrorDeg = UAMBallistics::StringErrorDeg(E->Def.Kind, E->bRodPlaced);
	RebuildStringVisual(*S);
	Say(FString::Printf(TEXT("String fixed: vertical %+.1f deg, horizontal %+.1f deg, +/- %.1f deg."), V, H, S->ErrorDeg));
	Changed();
	return true;
}

bool UAMCaseSubsystem::SetStringAngles(AAMEvidenceActor* E, float VerticalDeg, float HorizontalDeg)
{
	if (!E) { return false; }
	FAMTrajectoryString* S = FindString(E->Def.Id);
	if (!S) { return RunString(E) && SetStringAngles(E, VerticalDeg, HorizontalDeg); }
	if (UAMBallistics::IsHorizontalSurface(E->Def.Normal))
	{
		// Floors and worktops: elevation above the surface, azimuth all the way round.
		S->VerticalDeg = FMath::Clamp(VerticalDeg, 0.f, 60.f);
		S->HorizontalDeg = FRotator::NormalizeAxis(HorizontalDeg);
	}
	else
	{
		S->VerticalDeg = FMath::Clamp(VerticalDeg, -35.f, 35.f);
		S->HorizontalDeg = FMath::Clamp(HorizontalDeg, -80.f, 80.f);
	}
	RebuildStringVisual(*S);
	Changed();
	return true;
}

bool UAMCaseSubsystem::RemoveString(AAMEvidenceActor* E)
{
	if (!E) { return false; }
	const int32 Index = Strings.IndexOfByPredicate([&](const FAMTrajectoryString& S) { return S.EvidenceId == E->Def.Id; });
	if (Index == INDEX_NONE) { return false; }
	if (Strings[Index].Visual) { Strings[Index].Visual->Destroy(); }
	Strings.RemoveAt(Index);
	Say(TEXT("String removed."));
	Changed();
	return true;
}

void UAMCaseSubsystem::RebuildStringVisual(FAMTrajectoryString& S)
{
	AAMEvidenceActor* E = FindEvidenceActor(S.EvidenceId);
	if (!E) { return; }
	S.Origin = E->Def.Position + E->Def.Normal * 0.2f;
	S.BackDirection = UAMBallistics::BackDirectionFromAngles(E->Def.Normal, E->Def.SurfaceRight, E->Def.SurfaceUp, S.VerticalDeg, S.HorizontalDeg);

	// The string runs until it meets something solid (a wall, a cupboard, the floor).
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AMString), false);
	for (AAMEvidenceActor* Other : Evidence) { Q.AddIgnoredActor(Other); }
	const FVector Start = S.Origin + S.BackDirection * 3.f;
	const FVector End = S.Origin + S.BackDirection * 700.f;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Q);
	const FVector Anchor = bHit ? Hit.ImpactPoint : End;
	S.LengthCm = FVector::Distance(S.Origin, Anchor);

	if (!S.Visual)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		S.Visual = GetWorld()->SpawnActor<AAMStringActor>(AAMStringActor::StaticClass(), FTransform(S.Origin), P);
	}
	if (AAMStringActor* SA = Cast<AAMStringActor>(S.Visual))
	{
		SA->Setup(S.Origin, Anchor);
	}
}

int32 UAMCaseSubsystem::PlaceFigure(FVector FloorPoint, float FacingYawDeg)
{
	if (Figures.Num() >= 4) { Say(TEXT("Four figures placed. Remove one first.")); return 0; }
	FAMFigure& F = Figures.AddDefaulted_GetRef();
	F.Number = NextFigure++;
	F.FloorPoint = FVector(FloorPoint.X, FloorPoint.Y, 0.f);
	F.YawDeg = FacingYawDeg;
	Say(FString::Printf(TEXT("Figure F%d placed. Its gun hand is over the floor point."), F.Number));
	Changed();
	return F.Number;
}

bool UAMCaseSubsystem::SetFigure(int32 Number, const FString& Identity, EAMPosture Posture)
{
	FAMFigure* F = Figures.FindByPredicate([&](const FAMFigure& X) { return X.Number == Number; });
	if (!F) { Say(FString::Printf(TEXT("No figure F%d."), Number)); return false; }
	F->Identity = Identity;
	F->Posture = Posture;
	Changed();
	return true;
}

bool UAMCaseSubsystem::RemoveFigure(int32 Number)
{
	const int32 Removed = Figures.RemoveAll([&](const FAMFigure& X) { return X.Number == Number; });
	if (Removed) { Changed(); }
	return Removed > 0;
}

void UAMCaseSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Case) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	for (AAMEvidenceActor* E : Evidence)
	{
		if (E && E->bSubmittedToLab && !E->bLabResultAnnounced && Now >= E->LabReadyTime)
		{
			E->bLabResultAnnounced = true;
			Say(FString::Printf(TEXT("Lab report back for #%d."), E->MarkerNumber));
			Changed();
		}
	}
	DrawOverlays();
}

void UAMCaseSubsystem::DrawOverlays() const
{
#if ENABLE_DRAW_DEBUG
	// Greybox visuals. Phase 3 replaces cones, rings and figures with proper meshes.
	UWorld* W = GetWorld();
	if (!W || !Case) { return; }

	for (const FAMTrajectoryString& S : Strings)
	{
		const float HalfAngle = FMath::DegreesToRadians(S.ErrorDeg);
		DrawDebugCone(W, S.Origin, S.BackDirection, S.LengthCm, HalfAngle, HalfAngle, 24, FColor(222, 74, 53, 60), false, -1.f, 0, 0.2f);
	}

	for (int32 i = 0; i < Strings.Num(); ++i)
	{
		for (int32 j = i + 1; j < Strings.Num(); ++j)
		{
			FVector PA, PB;
			float SA = 0.f, SB = 0.f;
			const FAMTrajectoryString& A = Strings[i];
			const FAMTrajectoryString& B = Strings[j];
			if (UAMBallistics::ClosestPointsBetweenRays(A.Origin, A.BackDirection, B.Origin, B.BackDirection, PA, PB, SA, SB)
				&& SA > 40.f && SB > 40.f && SA < A.LengthCm && SB < B.LengthCm
				&& FVector::Distance(PA, PB) < Case->Tolerances.ConvergenceRadiusCm)
			{
				DrawDebugCircle(W, (PA + PB) * 0.5f, 5.f, 28, FColor(240, 190, 46), false, -1.f, 0, 0.6f, FVector(1, 0, 0), FVector(0, 1, 0), false);
			}
		}
	}

	for (const FAMFigure& F : Figures)
	{
		const float Muzzle = MuzzleHeightCm(F.Posture);
		const float Height = F.Posture == EAMPosture::Standing ? 172.f : 125.f;
		const FVector Fwd = FRotator(0.f, F.YawDeg, 0.f).Vector();
		const FVector Body = F.FloorPoint - Fwd * 56.f;
		DrawDebugCapsule(W, Body + FVector(0, 0, Height * 0.5f), Height * 0.5f, 17.f, FQuat::Identity, FColor(127, 184, 222), false, -1.f, 0, 0.8f);
		DrawDebugLine(W, Body + FVector(0, 0, Muzzle), F.FloorPoint + FVector(0, 0, Muzzle), FColor(127, 184, 222), false, -1.f, 0, 2.f);
		DrawDebugSphere(W, F.FloorPoint + FVector(0, 0, Muzzle), 3.f, 10, FColor(240, 190, 46), false, -1.f, 0, 0.5f);
		DrawDebugString(W, Body + FVector(0, 0, Height + 18.f), FString::Printf(TEXT("F%d"), F.Number), nullptr, FColor(127, 184, 222), 0.f, true);
	}

	if (bShowTruth)
	{
		for (const FAMShot& Shot : Case->Shots)
		{
			const FAMShooter* Shooter = Case->FindShooter(Shot.Shooter);
			if (!Shooter) { continue; }
			const FColor C = Shot.Shooter == TEXT("R") ? FColor(240, 168, 58) : FColor(79, 195, 217);
			FVector Prev = Shooter->Muzzle;
			for (const FString& Id : Shot.EvidenceIds)
			{
				if (const FAMEvidenceDef* D = Case->FindEvidence(Id))
				{
					DrawDebugLine(W, Prev, D->Position, C, false, -1.f, 0, 0.8f);
					Prev = D->Position;
				}
			}
			DrawDebugLine(W, Prev, Prev + Shot.Direction * 150.f, C, false, -1.f, 0, 0.4f);
			DrawDebugString(W, FMath::Lerp(Shooter->Muzzle, Prev, 0.3f) + FVector(0, 0, 8), FString::FromInt(Shot.Order), nullptr, C, 0.f, true);
		}
		for (const FAMShooter& S : Case->Shooters)
		{
			DrawDebugSphere(W, S.Muzzle, 4.f, 12, FColor::White, false, -1.f, 0, 0.6f);
		}
	}
#endif
}

// ---------------- Entry / exit calls, glass edges, firearms ----------------

FVector UAMCaseSubsystem::ShotDirectionFor(const AAMEvidenceActor* E) const
{
	if (!E || !Case) { return FVector::ZeroVector; }
	const FAMShot* Shot = Case->FindShot(E->Def.ShotId);
	if (!Shot) { return FVector::ZeroVector; }
	// After a ricochet the bullet travels in a new direction: use the segment that arrives at this trace.
	const int32 Index = Shot->EvidenceIds.IndexOfByKey(E->Def.Id);
	if (Index > 0)
	{
		if (const FAMEvidenceDef* Prev = Case->FindEvidence(Shot->EvidenceIds[Index - 1]))
		{
			return (E->Def.Position - Prev->Position).GetSafeNormal();
		}
	}
	return Shot->Direction;
}

bool UAMCaseSubsystem::CycleCall(AAMEvidenceActor* E)
{
	if (!E || !E->IsCallable()) { Say(TEXT("Only surface defects and glass take an entry / exit call.")); return false; }
	const TArray<EAMDefectCall> Options = E->CallOptions();
	const int32 Current = Options.IndexOfByKey(E->Call);
	E->Call = Options[(Current + 1) % Options.Num()];
	Say(FString::Printf(TEXT("Called: %s."), *E->CallLabel(E->Call)));
	Changed();
	return true;
}

bool UAMCaseSubsystem::ExamineEdge(AAMEvidenceActor* E)
{
	if (!E || E->Def.Kind != EAMEvidenceKind::GlassHole) { Say(TEXT("Edge examination is for holes in glass.")); return false; }
	if (E->bEdgeExamined) { Say(TEXT("Already examined.")); return false; }
	E->bEdgeExamined = true;
	Say(TEXT("Edge examined with a loupe and side light."));
	Changed();
	return true;
}

bool UAMCaseSubsystem::RecordCondition(AAMEvidenceActor* E)
{
	if (!E || !E->IsWeapon()) { Say(TEXT("Recording condition is for firearms.")); return false; }
	if (E->bConditionRecorded) { Say(TEXT("Condition already recorded.")); return false; }
	if (E->bMadeSafe)
	{
		E->bConditionLost = true;
		Say(TEXT("Too late: the weapon has already been handled. Its as-found condition can no longer be recorded."));
		Changed();
		return false;
	}
	E->bConditionRecorded = true;
	Say(TEXT("Condition recorded and photographed as found, without touching the weapon."));
	Changed();
	return true;
}

bool UAMCaseSubsystem::MakeSafe(AAMEvidenceActor* E)
{
	if (!E || !E->IsWeapon()) { Say(TEXT("Only firearms are made safe.")); return false; }
	if (E->bMadeSafe) { Say(TEXT("Already unloaded and made safe.")); return false; }
	E->bMadeSafe = true;
	E->ShowMadeSafe();
	if (!E->bConditionRecorded)
	{
		E->bConditionLost = true;
		Say(TEXT("Made safe, but the condition was not recorded first: safety and action positions as found are lost."));
	}
	else
	{
		Say(TEXT("Unloaded and made safe. Contents logged."));
	}
	Changed();
	return true;
}

// ---------------- Report and scoring ----------------

FAMScoreResult UAMCaseSubsystem::SubmitReport()
{
	FAMScoreResult R;
	if (!Case) { return R; }

	auto Add = [&R](const FString& Label, const FString& Detail, int32 Points, int32 Max)
	{
		FAMScoreLine& L = R.Lines.AddDefaulted_GetRef();
		L.Label = Label; L.Detail = Detail; L.Points = Points; L.MaxPoints = Max;
		R.Total += Points;
	};

	TArray<AAMEvidenceActor*> Weapons;
	for (AAMEvidenceActor* E : Evidence) { if (E && E->IsWeapon()) { Weapons.Add(E); } }
	// Weights sum to 100. A case without firearms at the scene moves the handling points to evidence.
	const int32 HandlingMax = Weapons.Num() ? 8 : 0;
	const int32 EvidenceMax = 20 + (8 - HandlingMax);

	// Evidence coverage and decoys (-6 each).
	int32 Genuine = 0, InReport = 0;
	TArray<FString> DecoysIn;
	for (const AAMEvidenceActor* E : Evidence)
	{
		if (!E) { continue; }
		if (E->Def.Role == EAMEvidenceRole::Genuine)
		{
			++Genuine;
			if (E->IsInReport()) { ++InReport; }
		}
		else if (E->Def.Role == EAMEvidenceRole::Decoy && E->IsInReport())
		{
			DecoysIn.Add(FString::Printf(TEXT("#%d %s"), E->MarkerNumber, *E->GetShortLabel().ToLower()));
		}
	}
	Add(TEXT("Evidence documented"), FString::Printf(TEXT("%d of %d genuine traces in the report"), InReport, Genuine),
		Genuine ? FMath::RoundToInt(float(EvidenceMax) * InReport / Genuine) : 0, EvidenceMax);
	Add(TEXT("Misleading items"), DecoysIn.Num() ? FString::Join(DecoysIn, TEXT("; ")) : TEXT("None in the report"), -6 * DecoysIn.Num(), 0);

	// Entry / exit / ricochet calls (10): every bullet-related surface trace, plus any decoy the player documented.
	int32 Callable = 0, Correct = 0;
	TArray<FString> Wrong;
	for (const AAMEvidenceActor* E : Evidence)
	{
		if (!E || !E->IsCallable()) { continue; }
		const bool bCounts = E->Def.Role == EAMEvidenceRole::Genuine || E->IsInReport();
		if (!bCounts) { continue; }
		++Callable;
		const EAMDefectCall Truth = E->TruthCall(ShotDirectionFor(E));
		if (E->IsInReport() && E->Call == Truth) { ++Correct; }
		else if (E->IsInReport() && E->Call != EAMDefectCall::Unclassified)
		{
			Wrong.Add(FString::Printf(TEXT("#%d called %s"), E->MarkerNumber, *E->CallLabel(E->Call).ToLower()));
		}
	}
	Add(TEXT("Entry, exit and ricochet calls"),
		FString::Printf(TEXT("%d of %d right%s%s"), Correct, Callable, Wrong.Num() ? TEXT("; wrong: ") : TEXT(""), *FString::Join(Wrong, TEXT(", "))),
		Callable ? FMath::RoundToInt(10.f * Correct / Callable) : 0, 10);

	// Firearm handling (8): condition recorded before making safe, then made safe.
	if (Weapons.Num())
	{
		float Points = 0.f;
		TArray<FString> Notes;
		const float Each = 8.f / Weapons.Num();
		for (const AAMEvidenceActor* W : Weapons)
		{
			const bool bProper = W->bConditionRecorded && W->bMadeSafe;
			Points += bProper ? Each : (W->bConditionRecorded ? Each * 0.5f : (W->bMadeSafe ? Each * 0.25f : 0.f));
			Notes.Add(FString::Printf(TEXT("%s: %s"), *W->GetShortLabel().ToLower(),
				bProper ? TEXT("recorded, then made safe") : W->bConditionLost ? TEXT("handled before recording") :
				W->bConditionRecorded ? TEXT("recorded, never made safe") : TEXT("untouched")));
		}
		Add(TEXT("Firearm handling"), FString::Join(Notes, TEXT("; ")), FMath::RoundToInt(Points), 8);
	}

	// Shooter positions (10 each).
	const FAMTolerances& Tol = Case->Tolerances;
	for (const FAMShooter& S : Case->Shooters)
	{
		TArray<const FAMFigure*> Candidates;
		for (const FAMFigure& F : Figures) { if (F.Identity == S.Key) { Candidates.Add(&F); } }
		if (Candidates.Num() == 0) { for (const FAMFigure& F : Figures) { if (F.Identity.IsEmpty()) { Candidates.Add(&F); } } }

		const FAMFigure* Best = nullptr;
		float BestD = TNumericLimits<float>::Max();
		for (const FAMFigure* F : Candidates)
		{
			const float D = FVector::Dist2D(F->FloorPoint, S.Muzzle);
			if (D < BestD) { BestD = D; Best = F; }
		}
		int32 P = 0;
		FString Detail = TEXT("No figure for this shooter");
		if (Best)
		{
			P = BestD <= Tol.ShooterFullCm ? 10 : BestD <= Tol.ShooterPartialCm ? 4 : 0;
			const bool bWrongPosture = Best->Posture != S.Posture;
			if (P > 0 && bWrongPosture) { P -= 3; }
			Detail = FString::Printf(TEXT("F%d is %.0f cm from the true muzzle position%s"), Best->Number, BestD, bWrongPosture ? TEXT(", wrong posture") : TEXT(""));
		}
		Add(FString::Printf(TEXT("%s's position"), *S.Name), Detail, P, 10);
	}

	// Shot count (8).
	const int32 TrueShots = Case->Shots.Num();
	const int32 SP = Claims.ShotCount == TrueShots ? 8 : (Claims.ShotCount >= 0 && FMath::Abs(Claims.ShotCount - TrueShots) == 1 ? 4 : 0);
	Add(TEXT("Shot count"), Claims.ShotCount < 0 ? TEXT("Not stated") : FString::Printf(TEXT("%d claimed, %d fired"), Claims.ShotCount, TrueShots), SP, 8);

	// Weapons (10).
	TSet<FString> TrueWeapons;
	for (const FAMShooter& S : Case->Shooters) { TrueWeapons.Add(S.WeaponKey); }
	int32 RightW = 0, WrongW = 0;
	for (const FString& K : Claims.WeaponKeys) { if (TrueWeapons.Contains(K)) { ++RightW; } else { ++WrongW; } }
	const int32 WP = (RightW == TrueWeapons.Num() && WrongW == 0) ? 10 : ((RightW == TrueWeapons.Num() || (RightW > 0 && WrongW == 0)) ? 5 : 0);
	Add(TEXT("Weapons"), Claims.WeaponKeys.Num() ? FString::Join(Claims.WeaponKeys, TEXT(", ")) : TEXT("Not stated"), WP, 10);

	// Firing order (12): full points need the sequence evidence in the report.
	const FAMShot* First = Case->FirstShot();
	bool bSequenceDocumented = Case->SequenceEvidenceIds.Num() > 0;
	for (const FString& Id : Case->SequenceEvidenceIds)
	{
		const AAMEvidenceActor* E = FindEvidenceActor(Id);
		bSequenceDocumented &= (E && E->IsInReport());
	}
	const bool bFirstRight = First && Claims.FirstShooter == First->Shooter;
	Add(TEXT("Who fired first"),
		bFirstRight ? (bSequenceDocumented ? TEXT("Correct, backed by the sequence evidence") : TEXT("Correct, but the evidence that proves it is not in the report"))
		            : (Claims.FirstShooter.IsEmpty() ? TEXT("Not stated") : TEXT("Incorrect")),
		bFirstRight ? (bSequenceDocumented ? 12 : 6) : 0, 12);

	// Statement (12).
	const bool bStatementRight = !Claims.StatementVerdict.IsEmpty() && Claims.StatementVerdict == Case->StatementTruth;
	Add(TEXT("Witness statement"), Claims.StatementVerdict.IsEmpty() ? TEXT("Not assessed") : (bStatementRight ? TEXT("Correct") : TEXT("Incorrect")),
		bStatementRight ? 12 : 0, 12);

	R.Total = FMath::Clamp(R.Total, 0, 100);
	R.Tier = R.Total >= 80 ? TEXT("Case closed") : R.Total >= 50 ? TEXT("Closed with doubts") : TEXT("Case collapsed");
	LastScore = R;
	bSubmitted = true;
	Say(FString::Printf(TEXT("Report scored: %d / 100, %s."), R.Total, *R.Tier));
	Changed();
	return R;
}

FString UAMCaseSubsystem::BuildReportText() const
{
	TArray<FString> Lines;
	TArray<const AAMEvidenceActor*> Docs;
	for (const AAMEvidenceActor* E : Evidence) { if (E && E->IsInReport()) { Docs.Add(E); } }
	Docs.Sort([](const AAMEvidenceActor& A, const AAMEvidenceActor& B) { return A.MarkerNumber < B.MarkerNumber; });

	Lines.Add(FString::Printf(TEXT("EVIDENCE (%d)"), Docs.Num()));
	for (const AAMEvidenceActor* E : Docs)
	{
		FString L = FString::Printf(TEXT("  #%d %s, %s"), E->MarkerNumber, *E->GetShortLabel(), *E->Def.Location.ToLower());
		if (E->IsCallable() && E->Call != EAMDefectCall::Unclassified) { L += TEXT(" | ") + E->CallLabel(E->Call); }
		if (E->Measurement.bValid) { L += FString::Printf(TEXT(" | %.1fx%.1f mm"), E->Measurement.WidthMm, E->Measurement.LengthMm); }
		if (E->LeadResult != EAMTestResult::NotTested) { L += E->LeadResult == EAMTestResult::Negative ? TEXT(" | Pb-") : TEXT(" | Pb+"); }
		if (E->CopperResult != EAMTestResult::NotTested) { L += E->CopperResult == EAMTestResult::Negative ? TEXT(" | Cu-") : TEXT(" | Cu+"); }
		if (E->bEdgeExamined) { L += TEXT(" | edge examined"); }
		if (E->bRodPlaced) { L += TEXT(" | rod"); }
		if (E->bProjectileRecovered) { L += TEXT(" | projectile"); }
		if (E->bPackaged) { L += TEXT(" | packaged"); }
		if (E->bSubmittedToLab) { L += GetWorld()->GetTimeSeconds() >= E->LabReadyTime ? TEXT(" | lab report") : TEXT(" | at lab"); }
		if (E->IsWeapon())
		{
			L += E->bConditionRecorded ? TEXT(" | condition recorded") : (E->bConditionLost ? TEXT(" | condition LOST") : TEXT(""));
			L += E->bMadeSafe ? TEXT(" | made safe") : TEXT(" | NOT made safe");
		}
		Lines.Add(L);
	}
	Lines.Add(FString::Printf(TEXT("STRINGS (%d)"), Strings.Num()));
	for (const FAMTrajectoryString& S : Strings)
	{
		const AAMEvidenceActor* E = FindEvidenceActor(S.EvidenceId);
		Lines.Add(FString::Printf(TEXT("  from #%d: V %+.1f  H %+.1f  +/-%.1f"), E ? E->MarkerNumber : 0, S.VerticalDeg, S.HorizontalDeg, S.ErrorDeg));
	}
	Lines.Add(FString::Printf(TEXT("FIGURES (%d)"), Figures.Num()));
	for (const FAMFigure& F : Figures)
	{
		Lines.Add(FString::Printf(TEXT("  F%d: %s, %s"), F.Number, F.Identity.IsEmpty() ? TEXT("unidentified") : *F.Identity,
			F.Posture == EAMPosture::Standing ? TEXT("standing") : TEXT("kneeling")));
	}
	Lines.Add(FString::Printf(TEXT("CLAIMS: shots %s | weapons %s | first %s | statement %s"),
		Claims.ShotCount < 0 ? TEXT("-") : *FString::FromInt(Claims.ShotCount),
		Claims.WeaponKeys.Num() ? *FString::Join(Claims.WeaponKeys, TEXT(",")) : TEXT("-"),
		Claims.FirstShooter.IsEmpty() ? TEXT("-") : *Claims.FirstShooter,
		Claims.StatementVerdict.IsEmpty() ? TEXT("-") : *Claims.StatementVerdict));
	return FString::Join(Lines, TEXT("\n"));
}

// ---------------- Packaging and the crime lab ----------------

bool UAMCaseSubsystem::Package(AAMEvidenceActor* E)
{
	if (!E) { return false; }
	if (!E->IsDocumented())
	{
		Say(TEXT("Photograph and mark it where it lies first. Nothing is moved before it is documented."));
		return false;
	}
	if (E->bPackaged) { Say(TEXT("Already packaged and sealed.")); return false; }
	if (E->IsWeapon() && !E->bMadeSafe)
	{
		Say(TEXT("Unload it and make it safe first. A loaded firearm is never packaged."));
		return false;
	}
	if (!E->IsCollectable())
	{
		Say(E->Def.Projectile != EAMProjectile::None
			? TEXT("Recover the bullet first (cut out the section around it; never pry it out with metal tools).")
			: TEXT("This stays in place: it is recorded by photographs, measurements and the sketch."));
		return false;
	}
	E->bPackaged = true;
	Say(E->GetPackagingNote());
	Changed();
	return true;
}

bool UAMCaseSubsystem::SubmitToLab(AAMEvidenceActor* E)
{
	if (!E) { return false; }
	if (!E->bPackaged) { Say(TEXT("Only packaged, sealed items go to the lab.")); return false; }
	if (E->bSubmittedToLab) { Say(TEXT("Already at the lab.")); return false; }
	if (LabRequestsLeft <= 0) { Say(TEXT("No lab requests left for this case.")); return false; }
	--LabRequestsLeft;
	E->bSubmittedToLab = true;
	E->LabReadyTime = GetWorld()->GetTimeSeconds() + (Case ? Case->LabTurnaroundSeconds : 45.f);
	Say(FString::Printf(TEXT("#%d sent to the lab. Report in about %.0f seconds."), E->MarkerNumber, Case ? Case->LabTurnaroundSeconds : 45.f));
	Changed();
	return true;
}

FString UAMCaseSubsystem::GetLabResult(const AAMEvidenceActor* E) const
{
	if (!E || !E->bSubmittedToLab || !Case) { return FString(); }
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < E->LabReadyTime)
	{
		return FString::Printf(TEXT("At the lab, report in %.0f s."), E->LabReadyTime - Now);
	}

	// Weapons whose test-fired standards are back from the lab, by weapon key.
	TMap<FString, const AAMEvidenceActor*> Standards;
	for (const AAMEvidenceActor* W : Evidence)
	{
		if (W && W->IsWeapon() && W->bSubmittedToLab && Now >= W->LabReadyTime)
		{
			Standards.Add(W->Def.Weapon.WeaponKey, W);
		}
	}
	auto WeaponKeyForShot = [this](const FString& ShotId) -> FString
	{
		const FAMShot* Shot = Case->FindShot(ShotId);
		const FAMShooter* Shooter = Shot ? Case->FindShooter(Shot->Shooter) : nullptr;
		return Shooter ? Shooter->WeaponKey : FString();
	};
	auto Comparison = [&Standards](const FString& Key, const TCHAR* What) -> FString
	{
		if (const AAMEvidenceActor* const* W = Standards.Find(Key))
		{
			return FString::Printf(TEXT(" Compared with test shots: %s identified as fired in %s #%d."), What,
				*(*W)->GetShortLabel().ToLower(), (*W)->MarkerNumber);
		}
		return Standards.Num()
			? FString::Printf(TEXT(" Compared with test shots: %s not fired in any weapon submitted so far."), What)
			: FString(TEXT(" No weapon submitted yet to compare against."));
	};

	switch (E->Def.Kind)
	{
	case EAMEvidenceKind::Pistol:
	case EAMEvidenceKind::Revolver:
		return FString::Printf(TEXT("Function test: %s fires normally; safeties and trigger work as designed, so an accidental "
			"discharge is not supported. Test shots fired and kept as comparison standards for %s."),
			*E->Def.Weapon.Description, *E->Def.Weapon.WeaponKey);
	case EAMEvidenceKind::CartridgeCase:
	{
		// Cases at the scene come from the pistol; key from the case's caliber.
		return FString(TEXT("9 mm Luger case; firing-pin, extractor and ejector marks recorded.")) + Comparison(TEXT("9mm"), TEXT("case"));
	}
	case EAMEvidenceKind::OldRimfireCase:
		return TEXT(".22 rimfire case, heavily corroded under the surface dirt: it was fired long before tonight. "
			"Its firing-pin mark does not match any weapon at the scene.");
	case EAMEvidenceKind::Clock:
		return TEXT("No bullet damage. The movement stopped when the clock hit the floor; the time shown is when it fell, not when any shot was fired.");
	default:
		break;
	}

	if (E->Def.Projectile != EAMProjectile::None)
	{
		FString Class;
		switch (E->Def.Projectile)
		{
		case EAMProjectile::Luger9mmFMJ:
			Class = TEXT("9 mm full metal jacket bullet; six lands and grooves, right twist."); break;
		case EAMProjectile::Special38LRN:
			Class = TEXT(".38 Special plain lead bullet; five lands and grooves, right twist."); break;
		case EAMProjectile::Special38LRNDeformed:
			Class = TEXT(".38 Special plain lead bullet, flattened on one side with stone particles embedded (ricochet); "
				"five lands and grooves, right twist, still readable on the undamaged side."); break;
		default: break;
		}
		return Class + Comparison(WeaponKeyForShot(E->Def.ShotId), TEXT("bullet"));
	}
	return TEXT("Nothing the lab can add.");
}
