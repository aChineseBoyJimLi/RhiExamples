#pragma once
#include "Win32Base.h"
#include "Log.h"
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

class GpuQueryDx : public Win32Base
{
public:
	
private:
	Microsoft::WRL::ComPtr<IDXGIFactory2> m_FactoryHandle;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_AdapterHandle;
	Microsoft::WRL::ComPtr<ID3D12Device5> m_DeviceHandle;
};