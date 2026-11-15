// Aftermath — pure ballistics math. No world access; covered by automation tests.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AMTypes.h"
#include "AMBallistics.generated.h"

UCLASS()
class AFTERMATHCORE_API UAMBallistics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Impact angle to the surface from an elliptical defect: arcsin(width / length), in degrees. */
	UFUNCTION(BlueprintPure, Category = "Aftermath|Ballistics")
	static float ImpactAngleFromEllipse(float WidthMm, float LengthMm);

	/**
	 * Direction from a defect back toward the shooter, from the impact angle and the
	 * long-axis direction (0 deg = SurfaceRight, 90 deg = SurfaceUp; the tail points the way the bullet travelled).
	 */
	UFUNCTION(BlueprintPure, Category = "Aftermath|Ballistics")
	static FVector BackDirectionFromEllipse(const FVector& Normal, const FVector& SurfaceRight, const FVector& SurfaceUp,
		float ImpactAngleDeg, float LongAxisDeg);

	/**
	 * Back direction from angle-finder readings. VerticalDeg is always elevation above the horizontal plane.
	 * Walls: HorizontalDeg is measured from the surface normal toward SurfaceRight.
	 * Floors and worktops (normal pointing up): HorizontalDeg is the azimuth in the surface, from SurfaceRight toward SurfaceUp.
	 */
	UFUNCTION(BlueprintPure, Category = "Aftermath|Ballistics")
	static FVector BackDirectionFromAngles(const FVector& Normal, const FVector& SurfaceRight, const FVector& SurfaceUp,
		float VerticalDeg, float HorizontalDeg);

	UFUNCTION(BlueprintPure, Category = "Aftermath|Ballistics")
	static void AnglesFromBackDirection(const FVector& Normal, const FVector& SurfaceRight, const FVector& SurfaceUp,
		const FVector& BackDirection, float& VerticalDeg, float& HorizontalDeg);

	/** True for floors and worktops, where angles are taken as elevation + azimuth. */
	static bool IsHorizontalSurface(const FVector& Normal) { return FMath::Abs(Normal.Z) > 0.7f; }

	/** Closest points between two rays (parameters >= 0). Returns false when the rays are parallel. */
	UFUNCTION(BlueprintPure, Category = "Aftermath|Ballistics")
	static bool ClosestPointsBetweenRays(const FVector& OriginA, const FVector& DirA, const FVector& OriginB, const FVector& DirB,
		FVector& PointA, FVector& PointB, float& DistanceAlongA, float& DistanceAlongB);

	/** Least-squares point nearest to all lines. RmsCm = average miss distance. Needs at least two non-parallel lines. */
	UFUNCTION(BlueprintPure, Category = "Aftermath|Ballistics")
	static bool ConvergencePoint(const TArray<FVector>& Origins, const TArray<FVector>& Directions, FVector& Point, float& RmsCm);

	/** A caliper reading of a defect, with noise seeded by the trace so the same hole always reads the same. */
	static FAMMeasurement MeasureDefect(const FAMEvidenceDef& Def);

	/** Angular uncertainty of a string, in degrees, given what it was set from. */
	static float StringErrorDeg(EAMEvidenceKind Kind, bool bFromRod);

	/** Gaussian sample from a seeded stream (Box-Muller). */
	static float Gaussian(FRandomStream& Stream);
};
