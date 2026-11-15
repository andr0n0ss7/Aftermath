#include "AMSurfaceLibrary.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogAMSurfaces, Log, All);

void UAMSurfaceLibrary::ReimportFromJson()
{
	ImportFromJsonFile(JsonFile.FilePath);
}

bool UAMSurfaceLibrary::ImportFromJsonFile(const FString& InPath)
{
	FString Path = InPath;
	if (FPaths::IsRelative(Path))
	{
		Path = FPaths::Combine(FPaths::ProjectDir(), Path);
	}
	FString Text;
	FAMSurfaceLibraryJson Parsed;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonObjectConverter::JsonObjectStringToUStruct(Text, &Parsed, 0, 0))
	{
		UE_LOG(LogAMSurfaces, Error, TEXT("Surface library import failed: %s"), *Path);
		return false;
	}
	Modify();
	Surfaces = Parsed.Surfaces;
	JsonFile.FilePath = InPath;
	MarkPackageDirty();
	UE_LOG(LogAMSurfaces, Log, TEXT("Surface library: %d materials"), Surfaces.Num());
	return true;
}

const FAMSurfaceProfile* UAMSurfaceLibrary::Find(const FString& Id) const
{
	return Surfaces.FindByPredicate([&](const FAMSurfaceProfile& S) { return S.Id == Id; });
}
