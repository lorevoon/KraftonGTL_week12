#pragma once
#include <string>
#include <unordered_map>
#include <d3d11.h>

/**
 * @brief 썸네일 데이터 구조체
 */
struct FThumbnailData
{
	ID3D11ShaderResourceView* SRV = nullptr;
	ID3D11Texture2D* Texture = nullptr;
	int Width = 0;
	int Height = 0;
	bool bOwnedByManager = true;  // true면 Manager가 Release 책임, false면 외부(ResourceManager)가 관리
};

/**
 * @brief 파일 썸네일 관리 클래스
 * - FBX, Prefab 등의 파일에 대한 미리보기 텍스처 생성 및 캐싱
 * - ImGui::Image()에서 사용 가능한 ShaderResourceView 제공
 */
class FThumbnailManager
{
public:
	static FThumbnailManager& GetInstance()
	{
		static FThumbnailManager Instance;
		return Instance;
	}

	/**
	 * @brief 초기화
	 * @param InDevice D3D11 디바이스
	 * @param InDeviceContext D3D11 디바이스 컨텍스트
	 */
	void Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext);

	/**
	 * @brief 정리
	 */
	void Shutdown();

	/**
	 * @brief 파일 경로에 대한 썸네일 가져오기 (없으면 생성)
	 * @param FilePath 파일 경로
	 * @return 썸네일 ShaderResourceView (ImGui::Image에 사용)
	 */
	ID3D11ShaderResourceView* GetThumbnail(const std::string& FilePath);

	/**
	 * @brief 캐시 초기화
	 */
	void ClearCache();

private:
	FThumbnailManager() = default;
	~FThumbnailManager() { Shutdown(); }

	// 복사/이동 방지 (싱글톤)
	FThumbnailManager(const FThumbnailManager&) = delete;
	FThumbnailManager& operator=(const FThumbnailManager&) = delete;

	/**
	 * @brief FBX 파일용 썸네일 생성
	 */
	FThumbnailData* CreateFBXThumbnail(const std::string& FilePath);

	/**
	 * @brief 기본 아이콘 텍스처 생성 (Prefab 등)
	 */
	FThumbnailData* CreateDefaultThumbnail(const std::string& Extension);

	/**
	 * @brief 이미지 파일용 썸네일 생성
	 */
	FThumbnailData* CreateImageThumbnail(const std::string& FilePath);

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;

	// 썸네일 캐시 (파일 경로 -> 썸네일 데이터)
	std::unordered_map<std::string, FThumbnailData> ThumbnailCache;

	// 기본 아이콘 캐시 (확장자 -> 썸네일 데이터)
	std::unordered_map<std::string, FThumbnailData> DefaultIconCache;

	// 썸네일 크기
	static constexpr int ThumbnailSize = 128;
};
