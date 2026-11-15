// Run in the editor: Tools > Session Frontend > Automation, filter "Aftermath".
#include "AMBallistics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAMBallisticsAngleTest, "Aftermath.Ballistics.ImpactAngle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAMBallisticsAngleTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Round hole is perpendicular"), UAMBallistics::ImpactAngleFromEllipse(9.f, 9.f), 90.f, 0.01f);
	TestEqual(TEXT("2:1 ellipse is 30 degrees"), UAMBallistics::ImpactAngleFromEllipse(9.f, 18.f), 30.f, 0.01f);
	TestEqual(TEXT("Width larger than length is clamped"), UAMBallistics::ImpactAngleFromEllipse(10.f, 9.f), 90.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAMBallisticsDirectionTest, "Aftermath.Ballistics.BackDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAMBallisticsDirectionTest::RunTest(const FString& Parameters)
{
	// Wall facing -X (kitchen window case). Basis exported from the case data.
	const FVector N(-1, 0, 0), R(0, 1, 0), U(0, 0, 1);
	// Shot R1 in Case_Kitchen0214: travels (0.9069, -0.4162, 0.0657), impact 65.08 deg, long axis 171.03 deg.
	const FVector Travel = FVector(0.906892, -0.416207, 0.065717).GetSafeNormal();
	const FVector Back = UAMBallistics::BackDirectionFromEllipse(N, R, U, 65.0794f, 171.0281f);
	TestTrue(TEXT("Ellipse reproduces the true back direction"), FVector::DotProduct(Back, -Travel) > 0.9999f);

	float V = 0.f, H = 0.f;
	UAMBallistics::AnglesFromBackDirection(N, R, U, Back, V, H);
	const FVector Again = UAMBallistics::BackDirectionFromAngles(N, R, U, V, H);
	TestTrue(TEXT("Angles round-trip"), FVector::DotProduct(Again, Back) > 0.99999f);

	// Ricochet E17 on the worktop (normal up): incoming travel from the case data, impact 8.31 deg, long axis 7.13 deg.
	const FVector WN(0, 0, 1), WR(1, 0, 0), WU(0, 1, 0);
	const FVector Incoming = (FVector(260, 360, 92) - FVector(-100, 315, 145)).GetSafeNormal();
	const FVector RicBack = UAMBallistics::BackDirectionFromEllipse(WN, WR, WU, 8.311f, 7.125f);
	TestTrue(TEXT("Ricochet mark points back to the shooter"), FVector::DotProduct(RicBack, -Incoming) > 0.9999f);
	UAMBallistics::AnglesFromBackDirection(WN, WR, WU, RicBack, V, H);
	TestTrue(TEXT("Horizontal-surface angles round-trip"),
		FVector::DotProduct(UAMBallistics::BackDirectionFromAngles(WN, WR, WU, V, H), RicBack) > 0.99999f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAMBallisticsConvergenceTest, "Aftermath.Ballistics.Convergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAMBallisticsConvergenceTest::RunTest(const FString& Parameters)
{
	const FVector Muzzle(300, 295, 140);
	TArray<FVector> O = {FVector(507, 200, 155), FVector(6, 220, 130), FVector(6, 75, 152)};
	TArray<FVector> D;
	for (const FVector& P : O) { D.Add((Muzzle - P).GetSafeNormal()); }
	FVector Point;
	float Rms = 0.f;
	TestTrue(TEXT("Solves"), UAMBallistics::ConvergencePoint(O, D, Point, Rms));
	TestTrue(TEXT("Finds the muzzle"), FVector::Distance(Point, Muzzle) < 0.1f);
	TestTrue(TEXT("Zero residual"), Rms < 0.1f);

	FVector PA, PB;
	float SA = 0.f, SB = 0.f;
	TestTrue(TEXT("Rays meet"), UAMBallistics::ClosestPointsBetweenRays(O[0], D[0], O[1], D[1], PA, PB, SA, SB));
	TestTrue(TEXT("At the muzzle"), FVector::Distance(PA, Muzzle) < 0.1f && FVector::Distance(PB, Muzzle) < 0.1f);
	return true;
}

#endif
