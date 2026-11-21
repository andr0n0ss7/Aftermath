// Aftermath — one case: briefing, ground truth shot record, every trace in the scene.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AMTypes.h"
#include "AMCaseData.generated.h"

/** JSON mirror of UAMCaseData, used only for import. Field names match the case JSON files. */
USTRUCT()
struct FAMCaseJson
{
	GENERATED_BODY()

	UPROPERTY() FString CaseId;
	UPROPERTY() FString Title;
	UPROPERTY() FString Briefing;
	UPROPERTY() FString Statement;
	UPROPERTY() FString StatementTruth;
	UPROPERTY() TArray<FAMShooter> Shooters;
	UPROPERTY() TArray<FAMShot> Shots;
	UPROPERTY() TArray<FAMEvidenceDef> Evidence;
	UPROPERTY() TArray<FString> SequenceEvidenceIds;
	UPROPERTY() int32 Swabs = 8;
	UPROPERTY() int32 LabRequests = 6;
	UPROPERTY() float LabTurnaroundSeconds = 45.f;
	UPROPERTY() FAMTolerances Tolerances;
};

UCLASS(BlueprintType)
class AFTERMATHCORE_API UAMCaseData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case") FString CaseId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case") FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case", meta = (MultiLine = true)) FText Briefing;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case", meta = (MultiLine = true)) FText Statement;
	/** "Consistent" or "Contradicted". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground truth") FString StatementTruth;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground truth") TArray<FAMShooter> Shooters;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground truth") TArray<FAMShot> Shots;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground truth") TArray<FAMEvidenceDef> Evidence;
	/** Traces that prove the firing order; all must be in the report for full sequence points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground truth") TArray<FString> SequenceEvidenceIds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules") int32 Swabs = 8;
	/** How many items the crime lab will examine for this case. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules") int32 LabRequests = 6;
	/** Real seconds until a lab result comes back. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules") float LabTurnaroundSeconds = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules") FAMTolerances Tolerances;

	/** Path to the case JSON, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Import") FFilePath JsonFile;

	/** Re-reads JsonFile and overwrites everything above. */
	UFUNCTION(CallInEditor, Category = "Import")
	void ReimportFromJson();

	/** Loads a case JSON file (absolute, or relative to the project folder). Returns false and logs on failure. */
	UFUNCTION(BlueprintCallable, Category = "Import")
	bool ImportFromJsonFile(const FString& Path);

	const FAMEvidenceDef* FindEvidence(const FString& Id) const;
	const FAMShot* FindShot(const FString& Id) const;
	const FAMShooter* FindShooter(const FString& Key) const;
	const FAMShot* FirstShot() const;
};
