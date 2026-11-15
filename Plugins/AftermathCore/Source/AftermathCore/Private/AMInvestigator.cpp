#include "AMInvestigator.h"
#include "AMCaseSubsystem.h"
#include "AMEvidenceActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "InputActionValue.h"

AAMInvestigator::AAMInvestigator()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(30.f, 88.f);
	BaseEyeHeight = 77.f;      // 165 cm eye height standing
	CrouchedEyeHeight = 32.f;  // ~82 cm eye height crouched

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	CMC->MaxWalkSpeed = WalkSpeed;
	CMC->MaxWalkSpeedCrouched = 75.f;
	CMC->GetNavAgentPropertiesRef().bCanCrouch = true;
	CMC->SetCrouchedHalfHeight(48.f);
	CMC->BrakingDecelerationWalking = 1400.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.f);

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(Camera);
	Flashlight->SetRelativeLocation(FVector(5.f, 16.f, -13.f));
	Flashlight->SetIntensityUnits(ELightUnits::Lumens);
	Flashlight->SetIntensity(700.f);
	Flashlight->SetLightColor(FLinearColor(1.f, 0.96f, 0.9f));
	Flashlight->SetInnerConeAngle(9.f);
	Flashlight->SetOuterConeAngle(24.f);
	Flashlight->SetAttenuationRadius(1000.f);
	Flashlight->SetCastShadows(true);
	Flashlight->SetSourceRadius(1.5f);

	bUseControllerRotationYaw = true;
}

UAMCaseSubsystem* AAMInvestigator::GetCase() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UAMCaseSubsystem>() : nullptr;
}

void AAMInvestigator::BuildInput()
{
	if (Mapping) { return; }
	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_Investigator"));

	auto MakeAction = [this](FName Name, EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this, Name);
		A->ValueType = Type;
		Actions.Add(Name, A);
		return A;
	};
	auto Swizzle = [this]() { return NewObject<UInputModifierSwizzleAxis>(this); };
	auto Negate = [this](bool X, bool Y)
	{
		UInputModifierNegate* N = NewObject<UInputModifierNegate>(this);
		N->bX = X; N->bY = Y; N->bZ = false;
		return N;
	};

	UInputAction* MoveA = MakeAction("IA_Move", EInputActionValueType::Axis2D);
	Mapping->MapKey(MoveA, EKeys::W).Modifiers.Add(Swizzle());
	{
		FEnhancedActionKeyMapping& S = Mapping->MapKey(MoveA, EKeys::S);
		S.Modifiers.Add(Swizzle());
		S.Modifiers.Add(Negate(true, true));
	}
	Mapping->MapKey(MoveA, EKeys::A).Modifiers.Add(Negate(true, false));
	Mapping->MapKey(MoveA, EKeys::D);

	UInputAction* LookA = MakeAction("IA_Look", EInputActionValueType::Axis2D);
	Mapping->MapKey(LookA, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));

	const TArray<TPair<FName, FKey>> Buttons = {
		{"IA_Fast", EKeys::LeftShift}, {"IA_Crouch", EKeys::C}, {"IA_Light", EKeys::F}, {"IA_Inspect", EKeys::E},
		{"IA_Inspect", EKeys::LeftMouseButton},
		{"IA_Document", EKeys::One}, {"IA_Measure", EKeys::Two}, {"IA_Lead", EKeys::Three}, {"IA_Copper", EKeys::Four},
		{"IA_Rod", EKeys::Five}, {"IA_Recover", EKeys::Six}, {"IA_String", EKeys::Seven}, {"IA_Exclude", EKeys::X},
		{"IA_Call", EKeys::Eight}, {"IA_Examine", EKeys::Nine}, {"IA_MakeSafe", EKeys::Zero},
		{"IA_Figure", EKeys::G}, {"IA_Report", EKeys::Tab}, {"IA_Submit", EKeys::Enter}
	};
	for (const TPair<FName, FKey>& B : Buttons)
	{
		UInputAction* A = Actions.Contains(B.Key) ? Actions[B.Key].Get() : MakeAction(B.Key, EInputActionValueType::Boolean);
		Mapping->MapKey(A, B.Value);
	}
}

void AAMInvestigator::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("AAMInvestigator needs Enhanced Input: set Default Input Component Class to EnhancedInputComponent in Project Settings > Input."));
		return;
	}
	EIC->BindAction(Actions["IA_Move"], ETriggerEvent::Triggered, this, &AAMInvestigator::Move);
	EIC->BindAction(Actions["IA_Look"], ETriggerEvent::Triggered, this, &AAMInvestigator::Look);
	EIC->BindAction(Actions["IA_Fast"], ETriggerEvent::Started, this, &AAMInvestigator::FastOn);
	EIC->BindAction(Actions["IA_Fast"], ETriggerEvent::Completed, this, &AAMInvestigator::FastOff);
	EIC->BindAction(Actions["IA_Crouch"], ETriggerEvent::Started, this, &AAMInvestigator::ToggleCrouch);
	EIC->BindAction(Actions["IA_Light"], ETriggerEvent::Started, this, &AAMInvestigator::ToggleFlashlight);
	EIC->BindAction(Actions["IA_Inspect"], ETriggerEvent::Started, this, &AAMInvestigator::Inspect);
	EIC->BindAction(Actions["IA_Document"], ETriggerEvent::Started, this, &AAMInvestigator::ToolDocument);
	EIC->BindAction(Actions["IA_Measure"], ETriggerEvent::Started, this, &AAMInvestigator::ToolMeasure);
	EIC->BindAction(Actions["IA_Lead"], ETriggerEvent::Started, this, &AAMInvestigator::ToolLead);
	EIC->BindAction(Actions["IA_Copper"], ETriggerEvent::Started, this, &AAMInvestigator::ToolCopper);
	EIC->BindAction(Actions["IA_Rod"], ETriggerEvent::Started, this, &AAMInvestigator::ToolRod);
	EIC->BindAction(Actions["IA_Recover"], ETriggerEvent::Started, this, &AAMInvestigator::ToolRecover);
	EIC->BindAction(Actions["IA_String"], ETriggerEvent::Started, this, &AAMInvestigator::ToolString);
	EIC->BindAction(Actions["IA_Exclude"], ETriggerEvent::Started, this, &AAMInvestigator::ToolExclude);
	EIC->BindAction(Actions["IA_Call"], ETriggerEvent::Started, this, &AAMInvestigator::ToolCall);
	EIC->BindAction(Actions["IA_Examine"], ETriggerEvent::Started, this, &AAMInvestigator::ToolExamine);
	EIC->BindAction(Actions["IA_MakeSafe"], ETriggerEvent::Started, this, &AAMInvestigator::ToolMakeSafe);
	EIC->BindAction(Actions["IA_Figure"], ETriggerEvent::Started, this, &AAMInvestigator::ToolFigure);
	EIC->BindAction(Actions["IA_Report"], ETriggerEvent::Started, this, &AAMInvestigator::ToggleReport);
	EIC->BindAction(Actions["IA_Submit"], ETriggerEvent::Started, this, &AAMInvestigator::Submit);
}

void AAMInvestigator::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->ClearAllMappings();
			Sub->AddMappingContext(Mapping, 0);
		}
	}
}

void AAMInvestigator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Smooth eye height between standing and crouched.
	FVector CamLoc = Camera->GetRelativeLocation();
	CamLoc.Z = FMath::FInterpTo(CamLoc.Z, bIsCrouched ? CrouchedEyeHeight : BaseEyeHeight, DeltaSeconds, 10.f);
	Camera->SetRelativeLocation(CamLoc);

	Focused = nullptr;
	FVector ViewLoc;
	FRotator ViewRot;
	if (!Controller) { return; }
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AMFocus), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, ViewLoc + ViewRot.Vector() * ReachCm, ECC_Visibility, Q))
	{
		AAMEvidenceActor* E = Cast<AAMEvidenceActor>(Hit.GetActor());
		// Hidden traces only reveal themselves to a crouched investigator with the light on.
		if (E && E->Def.bHidden && !(bIsCrouched && Flashlight->IsVisible()))
		{
			E = nullptr;
		}
		Focused = E;
	}
}

void AAMInvestigator::Move(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), V.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), V.X);
}

void AAMInvestigator::Look(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(V.X);
	AddControllerPitchInput(V.Y);
}

void AAMInvestigator::FastOn() { GetCharacterMovement()->MaxWalkSpeed = FastSpeed; }
void AAMInvestigator::FastOff() { GetCharacterMovement()->MaxWalkSpeed = WalkSpeed; }

void AAMInvestigator::ToggleCrouch()
{
	if (bIsCrouched) { UnCrouch(); } else { Crouch(); }
}

void AAMInvestigator::ToggleFlashlight()
{
	Flashlight->ToggleVisibility();
}

bool AAMInvestigator::RequireSelected() const
{
	if (!Selected)
	{
		if (UAMCaseSubsystem* C = GetCase()) { C->OnMessage.Broadcast(TEXT("Inspect a trace first (E).")); }
		return false;
	}
	return true;
}

void AAMInvestigator::Inspect()
{
	if (Focused)
	{
		Selected = Focused;
	}
	else if (UAMCaseSubsystem* C = GetCase())
	{
		C->OnMessage.Broadcast(TEXT("Nothing to inspect here. Bring the dot onto a trace within arm's reach."));
	}
}

void AAMInvestigator::ToolDocument() { if (RequireSelected()) { GetCase()->Document(Selected, GetActorLocation()); } }
void AAMInvestigator::ToolMeasure()  { if (RequireSelected()) { GetCase()->Measure(Selected); } }
void AAMInvestigator::ToolLead()     { if (RequireSelected()) { GetCase()->TestLead(Selected); } }
void AAMInvestigator::ToolCopper()   { if (RequireSelected()) { GetCase()->TestCopper(Selected); } }
void AAMInvestigator::ToolRod()      { if (RequireSelected()) { GetCase()->PlaceRod(Selected); } }
void AAMInvestigator::ToolRecover()  { if (RequireSelected()) { GetCase()->RecoverProjectile(Selected); } }
void AAMInvestigator::ToolString()   { if (RequireSelected()) { GetCase()->RunString(Selected); } }
void AAMInvestigator::ToolExclude()  { if (RequireSelected()) { GetCase()->ToggleExcluded(Selected); } }
void AAMInvestigator::ToolCall()     { if (RequireSelected()) { GetCase()->CycleCall(Selected); } }
void AAMInvestigator::ToolMakeSafe() { if (RequireSelected()) { GetCase()->MakeSafe(Selected); } }

void AAMInvestigator::ToolExamine()
{
	if (!RequireSelected()) { return; }
	// One key, two procedures: edge examination for glass, hands-off condition record for firearms.
	if (Selected->IsWeapon()) { GetCase()->RecordCondition(Selected); }
	else { GetCase()->ExamineEdge(Selected); }
}

void AAMInvestigator::ToolFigure()
{
	FVector ViewLoc;
	FRotator ViewRot;
	Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AMFigure), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, ViewLoc + ViewRot.Vector() * 900.f, ECC_Visibility, Q) && Hit.ImpactNormal.Z > 0.8f)
	{
		GetCase()->PlaceFigure(Hit.ImpactPoint, ViewRot.Yaw);
	}
	else
	{
		GetCase()->OnMessage.Broadcast(TEXT("Aim at the floor where the shooter stood."));
	}
}

void AAMInvestigator::ToggleReport() { bReportOpen = !bReportOpen; }

void AAMInvestigator::Submit()
{
	if (bReportOpen) { GetCase()->SubmitReport(); }
	else { GetCase()->OnMessage.Broadcast(TEXT("Open the report (Tab) and press Enter to submit.")); }
}

// ---- Console commands ----
void AAMInvestigator::AMShots(int32 Count) { GetCase()->Claims.ShotCount = Count; GetCase()->OnStateChanged.Broadcast(); }

void AAMInvestigator::AMWeapon(const FString& Key)
{
	TArray<FString>& W = GetCase()->Claims.WeaponKeys;
	if (W.Contains(Key)) { W.Remove(Key); } else { W.Add(Key); }
	GetCase()->OnStateChanged.Broadcast();
}

void AAMInvestigator::AMFirst(const FString& ShooterKey) { GetCase()->Claims.FirstShooter = ShooterKey; }
void AAMInvestigator::AMStatement(const FString& Verdict) { GetCase()->Claims.StatementVerdict = Verdict; }

void AAMInvestigator::AMFigure(int32 Number, const FString& Identity, const FString& Posture)
{
	GetCase()->SetFigure(Number, Identity, Posture.Equals(TEXT("kneeling"), ESearchCase::IgnoreCase) ? EAMPosture::Kneeling : EAMPosture::Standing);
}

void AAMInvestigator::AMAngles(float VerticalDeg, float HorizontalDeg)
{
	if (RequireSelected()) { GetCase()->SetStringAngles(Selected, VerticalDeg, HorizontalDeg); }
}

void AAMInvestigator::AMAssisted(bool bOn) { GetCase()->bAssisted = bOn; }
void AAMInvestigator::AMTruth(bool bOn) { GetCase()->bShowTruth = bOn; }
