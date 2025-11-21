// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UIWindow.h"
#include <filesystem>
#include <vector>

// Forward declarations
class FThumbnailManager;

/**
 * @brief 파일 엔트리 구조체
 * 컨텐츠 브라우저에서 표시할 파일/폴더 정보
 */
struct FFileEntry
{
	std::filesystem::path Path;        // 전체 경로
	FString FileName;                  // 파일 이름
	FString Extension;                 // 확장자
	bool bIsDirectory;                 // 디렉토리 여부
	uintmax_t FileSize;               // 파일 크기 (바이트)

	FFileEntry()
		: bIsDirectory(false), FileSize(0)
	{}
};

/**
 * @brief 컨텐츠 브라우저 윈도우
 * Unreal Engine 스타일의 에셋 브라우저
 * - 파일 시스템 탐색
 * - 드래그 앤 드롭으로 뷰포트에 에셋 배치
 * - 더블클릭으로 에셋 뷰어 열기
 */
class UContentBrowserWindow : public UUIWindow
{
public:
	DECLARE_CLASS(UContentBrowserWindow, UUIWindow)

	UContentBrowserWindow();
	~UContentBrowserWindow() override;

	void Initialize() override;
	void Cleanup() override;

	// UIWindow를 상속받아 RenderWindow를 직접 구현
	void RenderWindow();

	/**
	 * @brief 특정 경로로 이동
	 */
	void NavigateToPath(const std::filesystem::path& NewPath);

	/**
	 * @brief 현재 디렉토리 새로고침
	 */
	void RefreshCurrentDirectory();

	/**
	 * @brief 전체 컨텐츠 렌더링 (2패널 레이아웃)
	 */
	void RenderContent();

private:
	/**
	 * @brief 상단 경로 바 렌더링 (Breadcrumb)
	 */
	void RenderPathBar();

	/**
	 * @brief 파일/폴더 그리드 뷰 렌더링
	 */
	void RenderContentGrid();

private:
	/**
	 * @brief 개별 파일 아이템 렌더링
	 * @param Entry 파일 엔트리
	 * @param Index 인덱스
	 * @param bUseThumbnails 썸네일 사용 여부
	 */
	void RenderFileItem(FFileEntry& Entry, int Index, bool bUseThumbnails = true);

	/**
	 * @brief 드래그 소스 처리
	 */
	void HandleDragSource(FFileEntry& Entry);

	/**
	 * @brief 더블클릭 처리
	 */
	void HandleDoubleClick(FFileEntry& Entry);

	/**
	 * @brief 확장자에 따른 아이콘 텍스트 반환
	 */
	const char* GetIconForFile(const FFileEntry& Entry) const;

	/**
	 * @brief 파일 크기를 읽기 쉬운 형태로 변환
	 */
	FString FormatFileSize(uintmax_t Size) const;

private:
	/**
	 * @brief 왼쪽 폴더 트리 렌더링
	 */
	void RenderFolderTree();

	/**
	 * @brief 재귀적으로 폴더 트리 노드 렌더링
	 */
	void RenderFolderTreeNode(const std::filesystem::path& FolderPath);

private:
	std::filesystem::path CurrentPath;        // 현재 탐색 중인 경로
	std::filesystem::path RootPath;           // 루트 경로 (Data/)
	TArray<FFileEntry> DisplayedFiles;        // 현재 표시 중인 파일 목록
	TArray<std::filesystem::path> FolderList; // 왼쪽 패널의 폴더 목록

	FFileEntry* SelectedFile;                 // 선택된 파일
	int SelectedIndex;                        // 선택된 인덱스

	double LastClickTime;                     // 더블클릭 감지용
	int LastClickedIndex;                     // 마지막 클릭된 아이템 인덱스

	float ThumbnailSize;                      // 썸네일 크기
	int ColumnsCount;                         // 그리드 컬럼 수

	// 레이아웃
	float LeftPanelWidth;                     // 왼쪽 패널 너비 (픽셀)
	float SplitterWidth;                      // 스플리터 너비
	bool bIsDraggingSplitter;                 // 스플리터 드래그 중

	// 필터 옵션
	bool bShowFolders;                        // 폴더 표시 여부
	bool bShowFiles;                          // 파일 표시 여부
	FString SearchFilter;                     // 검색 필터

	// 썸네일 옵션
	bool bUseThumbnails;                      // 썸네일 사용 여부
};
