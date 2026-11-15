#include "AMBallistics.h"

float UAMBallistics::ImpactAngleFromEllipse(float WidthMm, float LengthMm)
{
	if (LengthMm <= KINDA_SMALL_NUMBER)
	{
		return 90.f;
	}
	const float Ratio = FMath::Clamp(WidthMm / FMath::Max(LengthMm, WidthMm), 0.f, 1.f);
	return FMath::RadiansToDegrees(FMath::Asin(Ratio));
}

FVector UAMBallistics::BackDirectionFromEllipse(const FVector& Normal, const FVector& SurfaceRight, const FVector& SurfaceUp,
	float ImpactAngleDeg, float LongAxisDeg)
{
	const float A = FMath::DegreesToRadians(ImpactAngleDeg);
	const float P = FMath::DegreesToRadians(LongAxisDeg);
	const FVector B = Normal * FMath::Sin(A)
		- SurfaceRight * (FMath::Cos(A) * FMath::Cos(P))
		- SurfaceUp * (FMath::Cos(A) * FMath::Sin(P));
	return B.GetSafeNormal();
}

FVector UAMBallistics::BackDirectionFromAngles(const FVector& Normal, const FVector& SurfaceRight, const FVector& SurfaceUp,
	float VerticalDeg, float HorizontalDeg)
{
	const float V = FMath::DegreesToRadians(VerticalDeg);
	const float H = FMath::DegreesToRadians(HorizontalDeg);
	FVector B;
	if (IsHorizontalSurface(Normal))
	{
		B = (SurfaceRight * FMath::Cos(H) + SurfaceUp * FMath::Sin(H)) * FMath::Cos(V) + FVector::UpVector * FMath::Sin(V);
	}
	else
	{
		B = Normal * (FMath::Cos(V) * FMath::Cos(H))
			+ SurfaceRight * (FMath::Cos(V) * FMath::Sin(H))
			+ FVector::UpVector * FMath::Sin(V);
	}
	return B.GetSafeNormal();
}

void UAMBallistics::AnglesFromBackDirection(const FVector& Normal, const FVector& SurfaceRight, const FVector& SurfaceUp,
	const FVector& BackDirection, float& VerticalDeg, float& HorizontalDeg)
{
	const FVector B = BackDirection.GetSafeNormal();
	VerticalDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(B.Z, -1.f, 1.f)));
	if (IsHorizontalSurface(Normal))
	{
		HorizontalDeg = FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(B, SurfaceUp), FVector::DotProduct(B, SurfaceRight)));
	}
	else
	{
		HorizontalDeg = FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(B, SurfaceRight), FVector::DotProduct(B, Normal)));
	}
}

bool UAMBallistics::ClosestPointsBetweenRays(const FVector& OriginA, const FVector& DirA, const FVector& OriginB, const FVector& DirB,
	FVector& PointA, FVector& PointB, float& DistanceAlongA, float& DistanceAlongB)
{
	const FVector U = DirA.GetSafeNormal();
	const FVector V = DirB.GetSafeNormal();
	const FVector W0 = OriginA - OriginB;
	const double B = FVector::DotProduct(U, V);
	const double D = FVector::DotProduct(U, W0);
	const double E = FVector::DotProduct(V, W0);
	const double Den = 1.0 - B * B;
	if (Den < 1e-6)
	{
		return false;
	}
	DistanceAlongA = static_cast<float>(FMath::Max(0.0, (B * E - D) / Den));
	DistanceAlongB = static_cast<float>(FMath::Max(0.0, (E - B * D) / Den));
	PointA = OriginA + U * DistanceAlongA;
	PointB = OriginB + V * DistanceAlongB;
	return true;
}

bool UAMBallistics::ConvergencePoint(const TArray<FVector>& Origins, const TArray<FVector>& Directions, FVector& Point, float& RmsCm)
{
	const int32 N = FMath::Min(Origins.Num(), Directions.Num());
	if (N < 2)
	{
		return false;
	}

	// Solve sum(I - d d^T) p = sum(I - d d^T) o.
	double M[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
	double R[3] = {0, 0, 0};
	for (int32 i = 0; i < N; ++i)
	{
		const FVector D = Directions[i].GetSafeNormal();
		const double d[3] = {D.X, D.Y, D.Z};
		const double o[3] = {Origins[i].X, Origins[i].Y, Origins[i].Z};
		for (int32 r = 0; r < 3; ++r)
		{
			for (int32 c = 0; c < 3; ++c)
			{
				const double P = (r == c ? 1.0 : 0.0) - d[r] * d[c];
				M[r][c] += P;
				R[r] += P * o[c];
			}
		}
	}

	const double Det =
		M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) -
		M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
		M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
	if (FMath::Abs(Det) < 1e-9)
	{
		return false;
	}

	auto Solve = [&](int32 Col)
	{
		double T[3][3];
		for (int32 r = 0; r < 3; ++r)
		{
			for (int32 c = 0; c < 3; ++c)
			{
				T[r][c] = (c == Col) ? R[r] : M[r][c];
			}
		}
		return (T[0][0] * (T[1][1] * T[2][2] - T[1][2] * T[2][1]) -
			T[0][1] * (T[1][0] * T[2][2] - T[1][2] * T[2][0]) +
			T[0][2] * (T[1][0] * T[2][1] - T[1][1] * T[2][0])) / Det;
	};
	Point = FVector(Solve(0), Solve(1), Solve(2));

	double SumSq = 0.0;
	for (int32 i = 0; i < N; ++i)
	{
		const FVector D = Directions[i].GetSafeNormal();
		const FVector ToP = Point - Origins[i];
		SumSq += (ToP - D * FVector::DotProduct(ToP, D)).SizeSquared();
	}
	RmsCm = static_cast<float>(FMath::Sqrt(SumSq / N));
	return true;
}

float UAMBallistics::Gaussian(FRandomStream& Stream)
{
	float U = 0.f, V = 0.f;
	while (U <= KINDA_SMALL_NUMBER) { U = Stream.GetFraction(); }
	while (V <= KINDA_SMALL_NUMBER) { V = Stream.GetFraction(); }
	return FMath::Sqrt(-2.f * FMath::Loge(U)) * FMath::Cos(2.f * PI * V);
}

FAMMeasurement UAMBallistics::MeasureDefect(const FAMEvidenceDef& Def)
{
	FAMMeasurement M;
	switch (Def.Kind)
	{
	case EAMEvidenceKind::PlasterCrater:
		M.Note = TEXT("Crater edge is irregular; width is not a reliable caliber guide.");
		return M;
	case EAMEvidenceKind::DrywallExit:
		M.Note = TEXT("Torn exit; no clean margin to measure.");
		return M;
	case EAMEvidenceKind::PaintedHole:
		M.Note = TEXT("No open hole to measure.");
		return M;
	case EAMEvidenceKind::WallPlugHoles:
		M.Note = TEXT("Two round holes, 6.0 mm, both perpendicular to the wall.");
		return M;
	case EAMEvidenceKind::DrywallEntry:
	case EAMEvidenceKind::SteelHole:
	case EAMEvidenceKind::GlassHole:
	case EAMEvidenceKind::RicochetMark:
		break;
	default:
		M.Note = TEXT("Nothing to measure with calipers.");
		return M;
	}

	FRandomStream Stream(Def.Seed + 3);
	const float SinA = FMath::Max(FMath::Sin(FMath::DegreesToRadians(Def.ImpactAngleDeg)), 0.05f);
	const float TrueW = Def.DiameterMm;
	const float TrueL = Def.DiameterMm / SinA;
	// A ricochet smear has soft edges, so its width and length read less precisely than a hole's.
	const bool bRicochet = Def.Kind == EAMEvidenceKind::RicochetMark;
	M.WidthMm = TrueW + Gaussian(Stream) * (bRicochet ? 0.6f : 0.22f);
	M.LengthMm = FMath::Max(M.WidthMm, TrueL + Gaussian(Stream) * (bRicochet ? 3.0f : 0.28f));
	M.ImpactAngleDeg = ImpactAngleFromEllipse(M.WidthMm, M.LengthMm);
	const float AxisNoise = (Def.Kind == EAMEvidenceKind::GlassHole) ? 4.f : bRicochet ? 3.f : 2.2f;
	M.LongAxisDeg = Def.LongAxisDeg + Gaussian(Stream) * AxisNoise;
	M.bValid = true;
	return M;
}

float UAMBallistics::StringErrorDeg(EAMEvidenceKind Kind, bool bFromRod)
{
	if (bFromRod)
	{
		return 1.5f;
	}
	switch (Kind)
	{
	case EAMEvidenceKind::GlassHole: return 6.f;
	case EAMEvidenceKind::RicochetMark: return 5.f;
	case EAMEvidenceKind::SteelHole: return 3.5f;
	default: return 4.f;
	}
}
