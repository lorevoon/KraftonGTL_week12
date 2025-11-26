#include "pch.h"
#include "ParticleLoader.h"
#include "ResourceManager.h"
#include "ParticleSystem.h"
#include "GlobalConsole.h"
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

void FParticleLoader::Preload()
{
	fs::path DataDir = fs::current_path() / "Data" / "ParticleSystems";

	if (!fs::exists(DataDir) || !fs::is_directory(DataDir))
	{
		UE_LOG("FParticleLoader::Preload: Directory not found: %s",
			   WideToUTF8(DataDir.wstring()).c_str());
		return;
	}

	size_t LoadedCount = 0;
	TSet<FString> ProcessedFiles;

	for (const auto& Entry : fs::recursive_directory_iterator(DataDir))
	{
		if (!Entry.is_regular_file())
			continue;

		const fs::path& Path = Entry.path();
		FString Extension = WideToUTF8(Path.extension().wstring());
		std::transform(Extension.begin(), Extension.end(), Extension.begin(),
					   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (Extension == ".json")
		{
			FString PathStr = NormalizePath(WideToUTF8(Path.wstring()));

			if (ProcessedFiles.find(PathStr) == ProcessedFiles.end())
			{
				ProcessedFiles.insert(PathStr);
				UResourceManager::GetInstance().Load<UParticleSystem>(PathStr);
				++LoadedCount;
			}
		}
	}

	RESOURCE.SetParticleSystems();

	UE_LOG("FParticleLoader::Preload: Loaded %zu particle systems from %s",
		   LoadedCount, WideToUTF8(DataDir.wstring()).c_str());
}
