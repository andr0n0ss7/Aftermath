#include "AMGameFramework.h"
#include "AMCaseData.h"
#include "AMCaseSubsystem.h"
#include "AMEvidenceActor.h"
#include "AMInvestigator.h"
#include "Components/BillboardComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"

// ---------------- Game mode ----------------

AAMGameMode::AAMGameMode()
{
	DefaultPawnClass = AAMInvestigator::StaticClass();
	HUDClass = AAMDebugHUD::StaticClass();
}

void AAMGameMode::StartPlay()
{
	Super::StartPlay();
	if (!GIsEditor && GEngine)
	{
		UGameUserSettings* Settings = GEngine->GetGameUserSettings();
		if (Settings && Settings->GetLastCPUBenchmarkResult() < 0.f)
		{
			Settings->RunHardwareBenchmark();
			Settings->ApplyHardwareBenchmarkResults();
		}
	}
}

// ---------------- Case director ----------------

AAMCaseDirector::AAMCaseDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
}

void AAMCaseDirector::BeginPlay()
{
	Super::BeginPlay();
	if (UAMCaseSubsystem* Sub = GetWorld()->GetSubsystem<UAMCaseSubsystem>())
	{
		Sub->StartCase(Case);
	}
}

// ---------------- Greybox HUD ----------------

namespace AMHud
{
	static const FLinearColor Ink(0.89f, 0.91f, 0.91f);
	static const FLinearColor Quiet(0.58f, 0.63f, 0.65f);
	static const FLinearColor Marker(0.94f, 0.75f, 0.18f);
	static const FLinearColor Good(0.38f, 0.75f, 0.56f);

	static FString TestText(EAMTestResult R)
	{
		switch (R)
		{
		case EAMTestResult::Positive: return TEXT("positive");
		case EAMTestResult::Weak:     return TEXT("weak positive");
		case EAMTestResult::Negative: return TEXT("negative");
		default:                      return TEXT("not tested");
		}
	}
}

void AAMDebugHUD::BeginPlay()
{
	Super::BeginPlay();
	if (UAMCaseSubsystem* Sub = GetWorld()->GetSubsystem<UAMCaseSubsystem>())
	{
		Sub->OnMessage.AddDynamic(this, &AAMDebugHUD::HandleMessage);
	}
}

void AAMDebugHUD::HandleMessage(const FString& Message)
{
	LastMessage = Message;
	LastMessageTime = GetWorld()->GetTimeSeconds();
}

void AAMDebugHUD::Line(const FString& Text, float X, float& Y, const FLinearColor& Color, float Scale)
{
	UFont* Font = GEngine->GetMediumFont();
	DrawText(Text, Color, X, Y, Font, Scale);
	Y += 20.f * Scale;
}

void AAMDebugHUD::WrapLines(const FString& Text, float X, float& Y, const FLinearColor& Color)
{
	// Breaks at ~62 characters on a space.
	FString Rest = Text;
	while (!Rest.IsEmpty())
	{
		int32 Cut = Rest.Len() <= 62 ? Rest.Len() : 62;
		if (Cut < Rest.Len()) { int32 Space; if (Rest.Left(62).FindLastChar(' ', Space) && Space > 0) { Cut = Space; } }
		Line(Rest.Left(Cut).TrimStartAndEnd(), X, Y, Color);
		Rest = Rest.Mid(Cut).TrimStart();
	}
}

void AAMDebugHUD::DrawEvidenceTag(const FAMEvidenceTag& Tag, double Age, float W, float H)
{
	// A manila evidence tag that slides up, holds, and fades: about 3.5 seconds in all.
	const double Show = 3.5;
	if (Age < 0.0 || Age > Show) { return; }
	const float In = FMath::Clamp(float(Age / 0.25), 0.f, 1.f);
	const float Out = FMath::Clamp(float((Show - Age) / 0.5), 0.f, 1.f);
	const float A = FMath::Min(In, Out);
	const float TW = 380.f, TH = 230.f;
	const float X = W * 0.5f - TW * 0.5f;
	const float Y = H - TH - 150.f + (1.f - In) * 40.f;
	auto C = [A](float R, float G, float B) { return FLinearColor(R, G, B, A); };

	DrawRect(C(0.f, 0.f, 0.f), X + 5, Y + 6, TW, TH);              // shadow
	DrawRect(C(0.86f, 0.76f, 0.55f), X, Y, TW, TH);                 // manila card
	DrawRect(C(0.62f, 0.12f, 0.10f), X, Y, TW, 34);                 // red header band
	DrawRect(C(0.55f, 0.47f, 0.33f), X + 16, Y + 9, 16, 16);        // reinforced string hole
	DrawRect(C(0.86f, 0.76f, 0.55f), X + 20, Y + 13, 8, 8);
	UFont* Font = GEngine->GetMediumFont();
	DrawText(TEXT("EVIDENCE"), C(1.f, 0.96f, 0.9f), X + 44, Y + 7, Font, 1.2f);
	DrawText(FString::Printf(TEXT("ITEM %d"), Tag.ItemNumber), C(1.f, 0.96f, 0.9f), X + TW - 104, Y + 7, Font, 1.2f);

	const FLinearColor Ink = C(0.12f, 0.10f, 0.08f);
	const FLinearColor Pen = C(0.08f, 0.15f, 0.42f);   // handwritten entries in blue ink
	const FLinearColor Rule = C(0.55f, 0.47f, 0.33f);
	float LY = Y + 46.f;
	auto Field = [&](const TCHAR* Label, const FString& Value)
	{
		DrawText(Label, Ink, X + 14, LY, Font, 0.78f);
		DrawText(Value.Left(44), Pen, X + 124, LY - 2, Font, 0.95f);
		DrawLine(X + 120, LY + 18, X + TW - 14, LY + 18, Rule, 1.f);
		LY += 29.f;
	};
	Field(TEXT("CASE NO."), Tag.CaseNumber);
	Field(TEXT("DESCRIPTION"), Tag.Description);
	Field(TEXT("LOCATION"), Tag.Location);
	Field(TEXT("DATE / TIME"), Tag.DateTime);
	Field(TEXT("COLLECTED BY"), Tag.CollectedBy);
	Field(TEXT("RECEIVED BY"), FString());
}

void AAMDebugHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) { return; }

	UAMCaseSubsystem* Sub = GetWorld()->GetSubsystem<UAMCaseSubsystem>();
	AAMInvestigator* Me = Cast<AAMInvestigator>(GetOwningPawn());
	if (!Sub || !Me) { return; }

	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float CX = W * 0.5f, CY = H * 0.5f;

	// Crosshair: a dot, a yellow ring when on a trace.
	if (Me->Focused)
	{
		DrawRect(AMHud::Marker, CX - 6, CY - 1, 12, 2);
		DrawRect(AMHud::Marker, CX - 1, CY - 6, 2, 12);
		float Y = CY + 18.f;
		const FString Label = Me->Focused->GetShortLabel() + (Me->Focused->IsDocumented() ? FString::Printf(TEXT("  #%d"), Me->Focused->MarkerNumber) : FString());
		Line(Label + TEXT("   [E] inspect"), CX - 120.f, Y, AMHud::Ink);
	}
	else
	{
		DrawRect(AMHud::Ink, CX - 2, CY - 2, 4, 4);
	}

	// Top bar.
	float Y = 16.f;
	if (Sub->Case)
	{
		Line(FString::Printf(TEXT("CASE %s  %s"), *Sub->Case->CaseId, *Sub->Case->Title.ToString()), 20.f, Y, AMHud::Marker);
	}
	Line(FString::Printf(TEXT("Swabs %d   Lab requests %d   Documented %d   Strings %d   Figures %d   %s"),
		Sub->SwabsLeft, Sub->LabRequestsLeft, Sub->CountDocumented(), Sub->Strings.Num(), Sub->Figures.Num(),
		Sub->bAssisted ? TEXT("Assisted") : TEXT("Expert")), 20.f, Y, AMHud::Quiet);

	// Selected trace panel.
	if (AAMEvidenceActor* E = Me->Selected)
	{
		float PY = 90.f;
		const float PX = W - 520.f;
		DrawRect(FLinearColor(0.05f, 0.08f, 0.09f, 0.85f), PX - 14, PY - 10, 510, H - 200);
		Line(FString::Printf(TEXT("%s%s"), E->IsDocumented() ? *FString::Printf(TEXT("#%d "), E->MarkerNumber) : TEXT(""), *E->GetShortLabel()), PX, PY, AMHud::Ink, 1.3f);
		Line(E->GetMaterialLabel() + TEXT(" | ") + E->Def.Location, PX, PY, AMHud::Marker);

		WrapLines(E->GetObservation(), PX, PY, AMHud::Quiet);
		PY += 6.f;
		if (E->Measurement.bValid)
		{
			Line(FString::Printf(TEXT("Calipers  %.1f x %.1f mm   long axis %.0f deg"), E->Measurement.WidthMm, E->Measurement.LengthMm,
				FMath::Fmod(E->Measurement.LongAxisDeg + 360.f, 360.f)), PX, PY, AMHud::Ink);
			if (Sub->bAssisted) { Line(FString::Printf(TEXT("Impact angle  %.1f deg to the surface"), E->Measurement.ImpactAngleDeg), PX, PY, AMHud::Ink); }
		}
		else if (!E->Measurement.Note.IsEmpty())
		{
			Line(TEXT("Calipers  ") + E->Measurement.Note, PX, PY, AMHud::Ink);
		}
		if (E->LeadResult != EAMTestResult::NotTested) { Line(TEXT("Lead (rhodizonate)  ") + AMHud::TestText(E->LeadResult), PX, PY, AMHud::Ink); }
		if (E->CopperResult != EAMTestResult::NotTested) { Line(TEXT("Copper (DTO)  ") + AMHud::TestText(E->CopperResult), PX, PY, AMHud::Ink); }
		if (E->bRodPlaced) { Line(TEXT("Rod seated through both faces"), PX, PY, AMHud::Ink); }
		if (E->bProjectileRecovered) { WrapLines(TEXT("Projectile  ") + E->GetProjectileDescription(), PX, PY, AMHud::Ink); }
		if (E->IsCallable()) { Line(TEXT("Your call  ") + E->CallLabel(E->Call), PX, PY, AMHud::Marker); }
		if (E->bEdgeExamined) { WrapLines(TEXT("Edge  ") + Sub->DescribeGlassEdge(E), PX, PY, AMHud::Ink); }
		if (E->IsWeapon())
		{
			if (E->bConditionRecorded) { for (const FString& L : E->GetConditionLines()) { WrapLines(L, PX, PY, AMHud::Ink); } }
			if (E->bMadeSafe) { for (const FString& L : E->GetUnloadLines()) { WrapLines(L, PX, PY, AMHud::Ink); } }
		}
		if (E->bCollected) { Line(TEXT("Bagged and tagged"), PX, PY, AMHud::Ink); }
		const FString Lab = Sub->GetLabResult(E);
		if (!Lab.IsEmpty()) { WrapLines(TEXT("Lab  ") + Lab, PX, PY, AMHud::Ink); }
		PY += 6.f;
		Line(TEXT("[1] Mark  [P] Bag and tag  [L] Send to lab  [X] Exclude"), PX, PY, AMHud::Marker);
		if (E->IsWeapon())
		{
			Line(TEXT("[9] Record condition as found  [0] Unload and make safe"), PX, PY, AMHud::Marker);
		}
		else
		{
			Line(TEXT("[2] Calipers  [3] Lead  [4] Copper  [5] Rod  [6] Recover"), PX, PY, AMHud::Marker);
			Line(E->Def.Kind == EAMEvidenceKind::GlassHole ? TEXT("[7] String  [8] Call  [9] Examine edge") : TEXT("[7] String  [8] Call entry / exit / ricochet"), PX, PY, AMHud::Marker);
		}
	}

	// Report panel.
	if (Me->bReportOpen)
	{
		float RY = 90.f;
		DrawRect(FLinearColor(0.05f, 0.08f, 0.09f, 0.9f), 6, RY - 10, 640, H - 150);
		Line(TEXT("REPORT  (built from what you documented)"), 20.f, RY, AMHud::Marker, 1.2f);
		TArray<FString> Lines;
		Sub->BuildReportText().ParseIntoArrayLines(Lines);
		for (const FString& L : Lines) { Line(L, 20.f, RY, AMHud::Ink); }
		RY += 8.f;
		Line(TEXT("Console (~): AMShots 5 | AMWeapon 9mm | AMFirst R | AMStatement Contradicted"), 20.f, RY, AMHud::Quiet);
		Line(TEXT("AMFigure 1 R standing | AMAngles <v> <h> | AMAssisted 0 | AMTruth 1"), 20.f, RY, AMHud::Quiet);
		Line(TEXT("[Enter] submit"), 20.f, RY, AMHud::Marker);

		if (Sub->bSubmitted)
		{
			RY += 10.f;
			Line(FString::Printf(TEXT("SCORE %d / 100   %s"), Sub->LastScore.Total, *Sub->LastScore.Tier), 20.f, RY,
				Sub->LastScore.Total >= 80 ? AMHud::Good : AMHud::Marker, 1.4f);
			for (const FAMScoreLine& S : Sub->LastScore.Lines)
			{
				Line(FString::Printf(TEXT("%+3d  %s: %s"), S.Points, *S.Label, *S.Detail), 20.f, RY, AMHud::Ink);
			}
		}
	}

	DrawEvidenceTag(Sub->LastTag, GetWorld()->GetTimeSeconds() - Sub->LastTagTime, W, H);

	// Message line.
	if (GetWorld()->GetTimeSeconds() - LastMessageTime < 4.0)
	{
		float MY = H - 120.f;
		Line(LastMessage, CX - 300.f, MY, AMHud::Ink, 1.1f);
	}

	float HY = H - 40.f;
	Line(TEXT("WASD move  Shift fast  C crouch  F light  E inspect  G figure  Tab report"), 20.f, HY, AMHud::Quiet);
}
