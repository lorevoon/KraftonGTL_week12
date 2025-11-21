#include "pch.h"
#include "RenderSettings.h"

// 전역 스키닝 모드 static 변수 정의 (기본값: GPU 스키닝)
ESkinningMode URenderSettings::GlobalSkinningMode = ESkinningMode::ForceGPU;
