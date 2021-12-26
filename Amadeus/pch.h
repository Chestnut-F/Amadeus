#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "Common\d3dx12.h"
#include <D3Dcompiler.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#include "Common/AmadeusHelper.h"
#include "Common/StepTimer.h"
#include "Common/DeviceResources.h"
#include "Common/DescriptorManager.h"
#include "Common/DescriptorCache.h"