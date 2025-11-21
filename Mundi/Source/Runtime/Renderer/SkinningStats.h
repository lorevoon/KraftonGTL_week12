#pragma once

#include <cstdint>

// 전방 선언
class FGPUTimer;

/**
 * 스키닝 통계 구조체 (CPU/GPU 통합)
 * 현재 활성 스키닝 모드의 성능 메트릭을 저장합니다.
 * CPU 모드: BoneMatrixCalc, VertexSkinning, BufferUpload, DrawTime 모두 사용
 * GPU 모드: BoneMatrixCalc, BufferUpload(본 버퍼), DrawTime 사용 (VertexSkinning은 0)
 */
struct FSkinningStats
{
	// 스키닝 시간 (밀리초)
	double BoneMatrixCalcTimeMS = 0.0;      // 본 행렬 계산 시간 (CPU에서 항상 수행)
	double VertexSkinningTimeMS = 0.0;      // 버텍스 스키닝 계산 시간 (CPU 모드에서만 사용)
	double BufferUploadTimeMS = 0.0;        // 버퍼 업로드 시간 (CPU: 버텍스 버퍼, GPU: 본 버퍼)
	double DrawTimeMS = 0.0;                // GPU draw 시간

	// 스키닝 카운트
	uint32_t SkinnedMeshCount = 0;          // 스키닝된 메시 개수
	uint32_t TotalVertices = 0;             // 처리한 총 버텍스 수
	uint32_t TotalBones = 0;                // 처리한 총 본 수
	uint32_t BufferUpdateCount = 0;         // 버퍼 업데이트 횟수

	// 메모리 사용량 (바이트)
	uint64_t BufferMemory = 0;              // 버퍼 메모리 (CPU: 버텍스 버퍼, GPU: 본 버퍼)

	/**
	 * 모든 통계를 0으로 리셋
	 */
	void Reset()
	{
		BoneMatrixCalcTimeMS = 0.0;
		VertexSkinningTimeMS = 0.0;
		BufferUploadTimeMS = 0.0;
		DrawTimeMS = 0.0;
		SkinnedMeshCount = 0;
		TotalVertices = 0;
		TotalBones = 0;
		BufferUpdateCount = 0;
		BufferMemory = 0;
	}

	/**
	 * 총 스키닝 시간 계산
	 * @return 본 계산 + 버텍스 스키닝 + 버퍼 업로드 + GPU draw 시간
	 */
	double GetTotalTimeMS() const
	{
		return BoneMatrixCalcTimeMS + VertexSkinningTimeMS + BufferUploadTimeMS + DrawTimeMS;
	}

	/**
	 * 메시당 평균 시간 계산
	 */
	double GetAverageTimePerMeshMS() const
	{
		if (SkinnedMeshCount == 0) return 0.0;
		return GetTotalTimeMS() / static_cast<double>(SkinnedMeshCount);
	}
};


/**
 * 스키닝 통계 전역 매니저 (싱글톤)
 * UStatsOverlayD2D에서 접근할 수 있도록 전역 통계 제공
 */
class FSkinningStatManager
{
public:
	static FSkinningStatManager& GetInstance()
	{
		static FSkinningStatManager Instance;
		return Instance;
	}

	/**
	 * 매 프레임 렌더링 시작 시 호출하여 프레임 단위 통계 데이터를 초기화합니다.
	 */
	void ResetFrameStats()
	{
		CurrentStats.Reset();
	}

	/**
	 * 통계 조회
	 */
	const FSkinningStats& GetStats() const
	{
		return CurrentStats;
	}

	/**
	 * 수정 가능한 통계 참조 반환 (데이터 수집용)
	 */
	FSkinningStats& GetMutableStats()
	{
		return CurrentStats;
	}

	// === 스키닝 데이터 추가 메서드 ===

	void AddMesh(uint32_t VertexCount, uint32_t BoneCount, uint64_t BufferSize)
	{
		CurrentStats.SkinnedMeshCount++;
		CurrentStats.TotalVertices += VertexCount;
		CurrentStats.TotalBones += BoneCount;
		CurrentStats.BufferMemory += BufferSize;
	}

	void AddBoneMatrixCalcTime(double TimeMS)
	{
		CurrentStats.BoneMatrixCalcTimeMS += TimeMS;
	}

	void AddVertexSkinningTime(double TimeMS)
	{
		CurrentStats.VertexSkinningTimeMS += TimeMS;
	}

	void AddBufferUploadTime(double TimeMS)
	{
		CurrentStats.BufferUploadTimeMS += TimeMS;
		CurrentStats.BufferUpdateCount++;
	}

	void AddDrawTime(double TimeMS)
	{
		CurrentStats.DrawTimeMS += TimeMS;
	}

	// === GPU 타이머 관리 ===

	/**
	 * GPU 타이머 초기화 (Renderer에서 한 번만 호출)
	 */
	void InitializeGPUTimer(void* Device);

	/**
	 * GPU 타이머 시작 (RenderOpaquePass 전)
	 */
	void BeginGPUTimer(void* DeviceContext);

	/**
	 * GPU 타이머 종료 (RenderOpaquePass 후)
	 */
	void EndGPUTimer(void* DeviceContext);

	/**
	 * GPU 타이머 결과 가져오기 (다음 프레임에서 호출)
	 * @return GPU draw 시간 (밀리초), 준비되지 않았으면 -1.0
	 */
	double GetGPUDrawTimeMS(void* DeviceContext);

private:
	FSkinningStatManager() = default;
	~FSkinningStatManager(); // cpp에서 정의 (unique_ptr 완전한 타입 필요)
	FSkinningStatManager(const FSkinningStatManager&) = delete;
	FSkinningStatManager& operator=(const FSkinningStatManager&) = delete;

	FSkinningStats CurrentStats;

	// GPU 타이머 (전역에서 지속)
	FGPUTimer* GPUDrawTimer = nullptr;
	double LastGPUDrawTimeMS = 0.0;
};
