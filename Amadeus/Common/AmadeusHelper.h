#pragma once

namespace Amadeus
{
    inline std::wstring GetAssetFullPath(const std::wstring& assetName) 
    { 
        std::vector<WCHAR> path(512);
        UINT pathSize = static_cast<UINT>(path.size());

        DWORD size = GetModuleFileName(nullptr, path.data(), pathSize);

        if (size == 0 || size == pathSize)
        {
            // Method failed or path was truncated.
            throw std::exception();
        }

        WCHAR* lastSlash = wcsrchr(path.data(), L'\\');
        if (lastSlash)
        {
            *(lastSlash + 1) = L'\0';
        }

        return path.data() + assetName;
    }

    inline std::wstring String2WString(std::string& str)
    {
        int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), NULL, 0);
        std::wstring wstr(count, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], count);
        return wstr;
    }

    inline std::string WString2String(const std::wstring& wstr)
    {
        int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), NULL, 0, NULL, NULL);
        std::string str(count, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
        return str;
    }

    inline std::string HrToString(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(s_str);
    }

    class HrException : public std::runtime_error
    {
    public:
        HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
        HRESULT Error() const { return m_hr; }
    private:
        const HRESULT m_hr;
    };

#define SAFE_RELEASE(p) if (p) (p)->Release()

    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw HrException(hr);
        }
    }

    // Assign a name to the object to aid with debugging.
#if defined(_DEBUG) || defined(DBG)
    inline void SetName(ID3D12Object* pObject, LPCWSTR name)
    {
        pObject->SetName(name);
    }
    inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
    {
        WCHAR fullName[50];
        if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
        {
            pObject->SetName(fullName);
        }
    }
#else
    inline void SetName(ID3D12Object*, LPCWSTR)
    {
    }
    inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
    {
    }
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)
}