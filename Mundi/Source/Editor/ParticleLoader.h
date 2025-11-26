#pragma once
#include "UEContainer.h"

class FParticleLoader
{
public:
	// Data/ParticleSystems/ 디렉토리의 .json 파일을 ResourceManager에 preload
	static void Preload();
};
