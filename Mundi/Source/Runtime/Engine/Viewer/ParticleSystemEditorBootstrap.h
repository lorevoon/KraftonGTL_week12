#pragma once
class ViewerState;
class UWorld;
struct ID3D11Device;

class ParticleSystemEditorBootstrap
{
public:
    static ViewerState* CreateViewerState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice);
    static void DestroyViewerState(ViewerState*& State);
};

