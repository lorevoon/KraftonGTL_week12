#include "pch.h"
#include "FViewport.h"
#include "FViewportClient.h"

FViewport::FViewport()
{
}

FViewport::~FViewport()
{
	Cleanup();
}

bool FViewport::Initialize(float InStartX, float InStartY, float InSizeX, float InSizeY, ID3D11Device* Device)
{
	if (!Device)
		return false;

	D3DDevice = Device;
	D3DDevice->GetImmediateContext(&D3DDeviceContext);
	StartX = static_cast<uint32>(InStartX);
	StartY = static_cast<uint32>(InStartY);
	SizeX = static_cast<uint32>(InSizeX);
	SizeY = static_cast<uint32>(InSizeY);

	// 렌더 타겟 생성
	if (!CreateRenderTarget(SizeX, SizeY))
	{
		return false;
	}

	return true;
}

bool FViewport::CreateRenderTarget(uint32 Width, uint32 Height)
{
	if (!D3DDevice || Width == 0 || Height == 0)
		return false;

	// 기존 리소스 해제
	ReleaseRenderTarget();

	HRESULT hr;

	// 1. 렌더 타겟 텍스처 생성
	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = Width;
	TexDesc.Height = Height;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_DEFAULT;
	TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.MiscFlags = 0;

	hr = D3DDevice->CreateTexture2D(&TexDesc, nullptr, &ViewportTexture);
	if (FAILED(hr))
		return false;

	// 2. RTV 생성
	D3D11_RENDER_TARGET_VIEW_DESC RtvDesc = {};
	RtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	RtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RtvDesc.Texture2D.MipSlice = 0;

	hr = D3DDevice->CreateRenderTargetView(ViewportTexture, &RtvDesc, &ViewportRTV);
	if (FAILED(hr))
		return false;

	// 3. SRV 생성 (ImGui::Image용)
	D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MostDetailedMip = 0;
	SrvDesc.Texture2D.MipLevels = 1;

	hr = D3DDevice->CreateShaderResourceView(ViewportTexture, &SrvDesc, &ViewportSRV);
	if (FAILED(hr))
		return false;

	// 4. 깊이 버퍼 생성
	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = Width;
	DepthDesc.Height = Height;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.SampleDesc.Quality = 0;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthDesc.CPUAccessFlags = 0;
	DepthDesc.MiscFlags = 0;

	hr = D3DDevice->CreateTexture2D(&DepthDesc, nullptr, &ViewportDepthBuffer);
	if (FAILED(hr))
		return false;

	// 5. DSV 생성
	hr = D3DDevice->CreateDepthStencilView(ViewportDepthBuffer, nullptr, &ViewportDSV);
	if (FAILED(hr))
		return false;

	return true;
}

void FViewport::ReleaseRenderTarget()
{
	if (ViewportDSV)
	{
		ViewportDSV->Release();
		ViewportDSV = nullptr;
	}
	if (ViewportDepthBuffer)
	{
		ViewportDepthBuffer->Release();
		ViewportDepthBuffer = nullptr;
	}
	if (ViewportSRV)
	{
		ViewportSRV->Release();
		ViewportSRV = nullptr;
	}
	if (ViewportRTV)
	{
		ViewportRTV->Release();
		ViewportRTV = nullptr;
	}
	if (ViewportTexture)
	{
		ViewportTexture->Release();
		ViewportTexture = nullptr;
	}
}

void FViewport::Cleanup()
{
	// 렌더 타겟 리소스 해제
	ReleaseRenderTarget();

	if (D3DDeviceContext)
	{
		D3DDeviceContext->Release();
		D3DDeviceContext = nullptr;
	}

	D3DDevice = nullptr;
}

void FViewport::BeginRenderFrame()
{
	// 뷰포트 설정
 //   D3D11_VIEWPORT viewport = {};
 //   viewport.Width = static_cast<float>(SizeX);
 //   viewport.Height = static_cast<float>(SizeY);
 //   viewport.MinDepth = 0.0f;
 //   viewport.MaxDepth = 1.0f;
 //   viewport.TopLeftX = static_cast<float>(StartX);
 //   viewport.TopLeftY = static_cast<float>(StartY);
 //   D3DDeviceContext->RSSetViewports(1, &viewport);

	//// ✅ 디버그: 각 뷰포트가 설정한 viewport 출력
	//static int callCount = 0;
	//if (callCount++ % 60 == 0) // 60프레임마다 출력
	//{
	//	UE_LOG("[FViewport::BeginRenderFrame] Viewport: TopLeft(%.1f, %.1f), Size(%.1f x %.1f)", 
	//		viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height);
	//}
}

void FViewport::Render()
{
	if (!ViewportClient) return;
	BeginRenderFrame();

	ViewportClient->Draw(this);

	EndRenderFrame();
}


void FViewport::EndRenderFrame()
{
	// 렌더링 완료 후 처리할 작업이 있다면 여기에 추가
}

void FViewport::Resize(uint32 NewStartX, uint32 NewStartY, uint32 NewSizeX, uint32 NewSizeY)
{
	bool bSizeChanged = (SizeX != NewSizeX || SizeY != NewSizeY);

	StartX = NewStartX;
	StartY = NewStartY;
	SizeX = NewSizeX;
	SizeY = NewSizeY;

	// 크기가 변경되면 렌더 타겟 재생성
	if (bSizeChanged && NewSizeX > 0 && NewSizeY > 0)
	{
		CreateRenderTarget(NewSizeX, NewSizeY);
	}
}

void FViewport::ProcessMouseMove(int32 X, int32 Y)
{
	if (ViewportClient)
	{
		ViewportMousePosition.X = static_cast<float>(X);
		ViewportMousePosition.Y = static_cast<float>(Y);
		ViewportClient->MouseMove(this, X, Y);
	}
}

void FViewport::ProcessMouseButtonDown(int32 X, int32 Y, int32 Button)
{
	if (ViewportClient)
	{
		ViewportClient->MouseButtonDown(this, X, Y, Button);
	}
}

void FViewport::ProcessMouseButtonUp(int32 X, int32 Y, int32 Button)
{
	if (ViewportClient)
	{
		ViewportClient->MouseButtonUp(this, X, Y, Button);
	}
}

void FViewport::ProcessKeyDown(int32 KeyCode)
{
	if (ViewportClient)
	{
		ViewportClient->KeyDown(this, KeyCode);
	}
}

void FViewport::ProcessKeyUp(int32 KeyCode)
{
	if (ViewportClient)
	{
		ViewportClient->KeyUp(this, KeyCode);
	}
}
