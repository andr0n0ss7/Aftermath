#include "AMCaseData.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogAMCase, Log, All);

void UAMCaseData::ReimportFromJson()
{
	ImportFromJsonFile(JsonFile.FilePath);
}

bool UAMCaseData::ImportFromJsonFile(const FString& InPath)
{
	FString Path = InPath;
	if (FPaths::IsRelative(Path))
	{
		Path = FPaths::Combine(FPaths::ProjectDir(), Path);
	}

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogAMCase, Error, TEXT("Case import: cannot read %s"), *Path);
		return false;
	}

	FAMCaseJson Parsed;
	if (!FJsonObjectConverter::JsonObjectStringToUStruct(Text, &Parsed, 0, 0))
	{
		UE_LOG(LogAMCase, Error, TEXT("Case import: %s is not valid case JSON"), *Path);
		return false;
	}

	Modify();
	CaseId = Parsed.CaseId;
	Title = FText::FromString(Parsed.Title);
	Briefing = FText::FromString(Parsed.Briefing);
	Statement = FText::FromString(Parsed.Statement);
	StatementTruth = Parsed.StatementTruth;
	Shooters = Parsed.Shooters;
	Shots = Parsed.Shots;
	Evidence = Parsed.Evidence;
	SequenceEvidenceIds = Parsed.SequenceEvidenceIds;
	Swabs = Parsed.Swabs;
	LabRequests = Parsed.LabRequests;
	LabTurnaroundSeconds = Parsed.LabTurnaroundSeconds;
	Tolerances = Parsed.Tolerances;
	JsonFile.FilePath = InPath;

	for (FAMShot& Shot : Shots)
	{
		Shot.Direction = Shot.Direction.GetSafeNormal();
	}
	for (FAMEvidenceDef& Def : Evidence)
	{
		Def.Normal = Def.Normal.GetSafeNormal();
		Def.SurfaceRight = Def.SurfaceRight.GetSafeNormal();
		Def.SurfaceUp = Def.SurfaceUp.GetSafeNormal();
	}

	MarkPackageDirty();
	UE_LOG(LogAMCase, Log, TEXT("Case %s imported: %d shooters, %d shots, %d traces"),
		*CaseId, Shooters.Num(), Shots.Num(), Evidence.Num());
	return true;
}

const FAMEvidenceDef* UAMCaseData::FindEvidence(const FString& Id) const
{
	return Evidence.FindByPredicate([&](const FAMEvidenceDef& D) { return D.Id == Id; });
}

const FAMShot* UAMCaseData::FindShot(const FString& Id) const
{
	return Shots.FindByPredicate([&](const FAMShot& S) { return S.Id == Id; });
}

const FAMShooter* UAMCaseData::FindShooter(const FString& Key) const
{
	return Shooters.FindByPredicate([&](const FAMShooter& S) { return S.Key == Key; });
}

const FAMShot* UAMCaseData::FirstShot() const
{
	const FAMShot* Best = nullptr;
	for (const FAMShot& S : Shots)
	{
		if (!Best || S.Order < Best->Order)
		{
			Best = &S;
		}
	}
	return Best;
}
