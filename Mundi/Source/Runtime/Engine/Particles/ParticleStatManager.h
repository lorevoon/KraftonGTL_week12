#pragma once
#include "UEContainer.h"

class FParticleStatManager
{
public:
	static FParticleStatManager& GetInstance()
	{
		static FParticleStatManager Instance;
		return Instance;
	}

	void ResetFrameStats()
	{
		ActiveEmitters = 0;
		SpawnedParticles = 0;
		DrawCalls = 0;
	}

	int32 GetActiveEmitters() const { return ActiveEmitters; }
	int32 GetSpawnedParticles() const { return SpawnedParticles; } // 현재 프레임 활성 파티클 합계
	int32 GetDrawCalls() const { return DrawCalls; }

	void AddActiveEmitter(int32 Value) { ActiveEmitters += Value; }
	void AddSpawnedParticles(int32 Value) { SpawnedParticles += Value; } // 활성 파티클 합산용
	void AddDrawCall(int32 Value) { DrawCalls += Value; }

private:
	FParticleStatManager() = default;
	~FParticleStatManager() = default;

	// 싱글톤 패턴을 위해 복사 및 대입 금지
	FParticleStatManager(const FParticleStatManager&) = delete;
	FParticleStatManager& operator=(const FParticleStatManager&) = delete;
private:
	// --- 통계 데이터 멤버 변수 ---

	int32 ActiveEmitters = 0;
	int32 SpawnedParticles = 0;
	int32 DrawCalls = 0;
};
