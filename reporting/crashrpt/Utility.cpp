/************************************************************************************* 
This file is a part of CrashRpt library.
Copyright (c) 2003-2013 The CrashRpt project authors. All Rights Reserved.

Use of this source code is governed by a BSD-style license
that can be found in the License.txt file in the root of the source
tree. All contributing project authors may
be found in the Authors.txt file in the root of the source tree.
***************************************************************************************/

// File: Utility.cpp
// Description: Miscellaneous helper functions
// Authors: mikecarruth, zexspectrum
// Date: 

#include "stdafx.h"
#include "Utility.h"
#include "resource.h"
#include "strconv.h"

CString Utility::getAppName()
{
    TCHAR szFileName[_MAX_PATH];
    GetModuleFileName(NULL, szFileName, _MAX_FNAME);

    CString sAppName; // Extract from last '\' to '.'
    sAppName = szFileName;
    sAppName = sAppName.Mid(sAppName.ReverseFind(_T('\\')) + 1)
        .SpanExcluding(_T("."));

    return sAppName;
}

CString Utility::GetModuleName(HMODULE hModule)
{
    CString string;
    LPTSTR buf = string.GetBuffer(_MAX_PATH);
    GetModuleFileName(hModule, buf, _MAX_PATH);
    string.ReleaseBuffer();
    return string;
}

CString Utility::GetModulePath(HMODULE hModule)
{
    CString string;
    LPTSTR buf = string.GetBuffer(_MAX_PATH);
    GetModuleFileName(hModule, buf, _MAX_PATH);
    TCHAR* ptr = _tcsrchr(buf,'\\');
    if(ptr!=NULL)
        *(ptr)=0; // remove executable name
    string.ReleaseBuffer();
    return string;
}

int Utility::getTempDirectory(CString& strTemp)
{
    TCHAR* pszTempVar = NULL;

#if _MSC_VER<1400
    pszTempVar = _tgetenv(_T("TEMP"));
    strTemp = CString(pszTempVar);
#else
    size_t len = 0;
    errno_t err = _tdupenv_s(&pszTempVar, &len, _T("TEMP"));
    if(err!=0)
    {
        // Couldn't get environment variable TEMP    
        return 1;
    }
    strTemp = CString(pszTempVar);
    free(pszTempVar);
#endif    

    return 0;
}

CString Utility::getTempFileName()
{
    TCHAR szTempDir[MAX_PATH - 14]   = _T("");
    TCHAR szTempFile[MAX_PATH]       = _T("");

    if (GetTempPath(MAX_PATH - 14, szTempDir))
        GetTempFileName(szTempDir, getAppName(), 0, szTempFile);

    return szTempFile;
}

int Utility::GetSystemTimeUTC(CString& sTime)
{
    sTime.Empty();

    // Get system time in UTC format

    time_t cur_time;
    time(&cur_time);
    char szDateTime[64];

#if _MSC_VER<1400
    struct tm* timeinfo = gmtime(&cur_time);
    strftime(szDateTime, 64,  "%Y-%m-%dT%H:%M:%SZ", timeinfo);
#else
    struct tm timeinfo;
    gmtime_s(&timeinfo, &cur_time);
    strftime(szDateTime, 64,  "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
#endif

    sTime = szDateTime;

    return 0;
}

void Utility::UTC2SystemTime(CString sUTC, SYSTEMTIME& st)
{
    CString sYear = sUTC.Mid(0, 4);
    CString sMonth = sUTC.Mid(5, 2);
    CString sDay = sUTC.Mid(8, 2);
    CString sHour = sUTC.Mid(11, 2);
    CString sMin = sUTC.Mid(14, 2);
    CString sSec = sUTC.Mid(17, 2);

    SYSTEMTIME UtcTime;
    memset(&UtcTime, 0, sizeof(SYSTEMTIME));
    UtcTime.wYear = (WORD)_ttoi(sYear);
    UtcTime.wMonth = (WORD)_ttoi(sMonth);
    UtcTime.wDay = (WORD)_ttoi(sDay);
    UtcTime.wHour = (WORD)_ttoi(sHour);
    UtcTime.wMinute = (WORD)_ttoi(sMin);
    UtcTime.wSecond = (WORD)_ttoi(sSec);

    // Convert to local time
    SystemTimeToTzSpecificLocalTime(NULL, &UtcTime, &st);
}

int Utility::GenerateGUID(CString& sGUID)
{
    int status = 1;
    sGUID.Empty();

    strconv_t strconv;

    // Create GUID

    UCHAR *pszUuid = 0; 
    GUID *pguid = NULL;
    pguid = new GUID;
    if(pguid!=NULL)
    {
        HRESULT hr = CoCreateGuid(pguid);
        if(SUCCEEDED(hr))
        {
            // Convert the GUID to a string
            hr = UuidToStringA(pguid, &pszUuid);
            if(SUCCEEDED(hr) && pszUuid!=NULL)
            { 
                status = 0;
                sGUID = strconv.a2t((char*)pszUuid);
                RpcStringFreeA(&pszUuid);
            }
        }
        delete pguid; 
    }

    return status;
}

int Utility::GetOSFriendlyName(CString& sOSName)
{
    sOSName.Empty();
    CRegKey regKey;
    LONG lResult = regKey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), KEY_READ);
    if(lResult==ERROR_SUCCESS)
    {    
        TCHAR buf[1024];
        ULONG buf_size = 0;

        TCHAR* PRODUCT_NAME = _T("ProductName");
        TCHAR* CURRENT_BUILD_NUMBER = _T("CurrentBuildNumber");
        TCHAR* CSD_VERSION = _T("CSDVersion");

#pragma warning(disable:4996)

        buf_size = 1023;
        if(ERROR_SUCCESS == regKey.QueryValue(buf, PRODUCT_NAME, &buf_size))
        {
            sOSName += buf;
        }

        buf_size = 1023;
        if(ERROR_SUCCESS == regKey.QueryValue(buf, CURRENT_BUILD_NUMBER, &buf_size))
        {
            sOSName += _T(" Build ");
            sOSName += buf;
        }

        buf_size = 1023;
        if(ERROR_SUCCESS == regKey.QueryValue(buf, CSD_VERSION, &buf_size))
        {
            sOSName += _T(" ");
            sOSName += buf;
        }

#pragma warning(default:4996)

        regKey.Close();

        // If we are actually running inside Wine/CrossOver (e.g. on macOS or
        // Linux), append information about the emulation environment so that
        // the crash report shows where the application was really executed.
        CString sWineInfo;
        if(0 == GetWineEnvironment(sWineInfo))
        {
            sOSName += _T(" ");
            sOSName += sWineInfo;
        }

        return 0;
    }

    return 1;
}

// Wine-specific exports of kernel32.dll for converting between Unix and DOS paths.
typedef WCHAR * (__cdecl *PFN_WINE_GET_DOS_FILE_NAME)(const char *unix_name);
typedef char * (__cdecl *PFN_WINE_GET_UNIX_FILE_NAME)(const WCHAR *dos_name);

// Unix paths are UTF-8 encoded on macOS (and virtually all Linux systems).
static CString Utf8ToWide(const char* szUtf8)
{
    CString sWide;
    int nLen = MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, NULL, 0);
    if(nLen > 1)
    {
        MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, sWide.GetBuffer(nLen), nLen);
        sWide.ReleaseBuffer();
    }
    return sWide;
}

static std::string WideToUtf8(LPCWSTR szWide)
{
    int nLen = WideCharToMultiByte(CP_UTF8, 0, szWide, -1, NULL, 0, NULL, NULL);
    if(nLen <= 1)
        return std::string();
    std::string sUtf8(nLen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, szWide, -1, &sUtf8[0], nLen, NULL, NULL);
    sUtf8.resize(nLen - 1); // drop terminating zero
    return sUtf8;
}

// Returns the Unix path of the outermost app bundle (".../Name.app") that contains
// the given Unix path, or an empty string if the path is not inside a bundle.
static CString GetAppBundleFromUnixPath(const CString& sUnixPath)
{
    CString sLower = sUnixPath;
    sLower.MakeLower();
    int nPos = sLower.Find(L".app/contents");
    if(nPos < 0)
        return CString();
    return sUnixPath.Left(nPos + 4); // up to and including ".app"
}

// Reads <key>szKey</key><string>value</string> from an XML property list.
static std::string GetPlistString(const std::string& sPlist, const char* szKey)
{
    const char* szWhitespace = " \t\r\n";
    std::string sKeyTag = std::string("<key>") + szKey + "</key>";
    size_t nPos = sPlist.find(sKeyTag);
    if(nPos == std::string::npos)
        return std::string();
    nPos += sKeyTag.length();
    size_t nStart = sPlist.find_first_not_of(szWhitespace, nPos);
    if(nStart == std::string::npos || sPlist.compare(nStart, 8, "<string>") != 0)
        return std::string(); // value is not a <string>
    nStart += 8;
    size_t nEnd = sPlist.find("</string>", nStart);
    if(nEnd == std::string::npos)
        return std::string();
    std::string sValue = sPlist.substr(nStart, nEnd - nStart);
    size_t nFirst = sValue.find_first_not_of(szWhitespace);
    if(nFirst == std::string::npos)
        return std::string();
    return sValue.substr(nFirst, sValue.find_last_not_of(szWhitespace) - nFirst + 1);
}

// On macOS, Wine is usually embedded in an app bundle (e.g. an application wrapped
// with CodeWeavers' PortJump/CrossOver). The bundle (and the Wine engine inside it)
// is versioned independently of the Windows executable, so report the bundle name and
// the version from its Contents/Info.plist, e.g. "EasyCash&Tax.app 3.9.0".
// Returns 0 if a bundle was found (the version may be reported as unknown), 1 otherwise.
static int GetWineAppBundleInfo(CString& sBundleInfo)
{
    sBundleInfo.Empty();

    HMODULE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
    if(hKernel32 == NULL)
        return 1;
    PFN_WINE_GET_DOS_FILE_NAME pfnWineGetDosFileName =
        (PFN_WINE_GET_DOS_FILE_NAME)GetProcAddress(hKernel32, "wine_get_dos_file_name");
    PFN_WINE_GET_UNIX_FILE_NAME pfnWineGetUnixFileName =
        (PFN_WINE_GET_UNIX_FILE_NAME)GetProcAddress(hKernel32, "wine_get_unix_file_name");
    if(pfnWineGetDosFileName == NULL)
        return 1;

    // Candidate paths inside the bundle: CrossOver's engine root (CX_ROOT) and the Wine
    // prefix are passed from the Unix environment; the bottle itself may also reside
    // inside the bundle, so check the Unix path of the executable as well.
    CString asBundles[3];
    int nBundles = 0;
    LPCWSTR aszVars[] = { L"CX_ROOT", L"WINEPREFIX" };
    for(int i = 0; i < _countof(aszVars); i++)
    {
        WCHAR szValue[2048];
        DWORD dwLen = GetEnvironmentVariableW(aszVars[i], szValue, _countof(szValue));
        if(dwLen > 0 && dwLen < _countof(szValue))
        {
            asBundles[nBundles] = GetAppBundleFromUnixPath(szValue);
            if(!asBundles[nBundles].IsEmpty())
                nBundles++;
        }
    }
    WCHAR szExe[_MAX_PATH];
    if(pfnWineGetUnixFileName != NULL && GetModuleFileNameW(NULL, szExe, _MAX_PATH))
    {
        char* szUnixExe = pfnWineGetUnixFileName(szExe);
        if(szUnixExe != NULL)
        {
            asBundles[nBundles] = GetAppBundleFromUnixPath(Utf8ToWide(szUnixExe));
            if(!asBundles[nBundles].IsEmpty())
                nBundles++;
            HeapFree(GetProcessHeap(), 0, szUnixExe);
        }
    }

    for(int i = 0; i < nBundles; i++)
    {
        CString sName = asBundles[i].Mid(asBundles[i].ReverseFind(L'/') + 1);
        if(sBundleInfo.IsEmpty())
            sBundleInfo = sName + _T(" (version unknown)");

        CString sInfoPlist = asBundles[i] + L"/Contents/Info.plist";
        WCHAR* szDosPath = pfnWineGetDosFileName(WideToUtf8(sInfoPlist).c_str());
        if(szDosPath == NULL)
            continue;
        HANDLE hFile = CreateFileW(szDosPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        HeapFree(GetProcessHeap(), 0, szDosPath);
        if(hFile == INVALID_HANDLE_VALUE)
            continue;

        std::string sPlist;
        DWORD dwSize = GetFileSize(hFile, NULL);
        if(dwSize != INVALID_FILE_SIZE && dwSize > 0 && dwSize < 1024 * 1024)
        {
            DWORD dwRead = 0;
            sPlist.resize(dwSize);
            if(!ReadFile(hFile, &sPlist[0], dwSize, &dwRead, NULL))
                dwRead = 0;
            sPlist.resize(dwRead);
        }
        CloseHandle(hFile);

        std::string sVersion = GetPlistString(sPlist, "CFBundleShortVersionString");
        if(sVersion.empty())
            sVersion = GetPlistString(sPlist, "CFBundleVersion");
        if(!sVersion.empty())
        {
            sBundleInfo = sName + _T(" ") + Utf8ToWide(sVersion.c_str());
            break;
        }
    }

    return sBundleInfo.IsEmpty() ? 1 : 0;
}

int Utility::GetWineEnvironment(CString& sWineInfo)
{
    sWineInfo.Empty();

    // Presence of the HKEY_LOCAL_MACHINE\Software\Wine registry key indicates
    // that the application is running under Wine (or a Wine-based product such
    // as CrossOver). On a native Windows installation this key does not exist.
    CRegKey regKey;
    if(ERROR_SUCCESS != regKey.Open(HKEY_LOCAL_MACHINE, _T("Software\\Wine"), KEY_READ))
        return 1;
    regKey.Close();

    // Wine exposes its version and the host operating system through a set of
    // (Wine-specific) exports of ntdll.dll. These are absent on real Windows,
    // so we resolve them dynamically.
    typedef const char * (__cdecl *PFN_WINE_GET_VERSION)(void);
    typedef const char * (__cdecl *PFN_WINE_GET_BUILD_ID)(void);
    typedef void (__cdecl *PFN_WINE_GET_HOST_VERSION)(const char **sysname, const char **release);

    HMODULE hNtdll = GetModuleHandle(_T("ntdll.dll"));
    if(hNtdll == NULL)
        return 1;

    PFN_WINE_GET_VERSION pfnWineGetVersion =
        (PFN_WINE_GET_VERSION)GetProcAddress(hNtdll, "wine_get_version");
    if(pfnWineGetVersion == NULL)
        return 1; // The registry key exists but the export does not -> not a genuine Wine.

    PFN_WINE_GET_BUILD_ID pfnWineGetBuildId =
        (PFN_WINE_GET_BUILD_ID)GetProcAddress(hNtdll, "wine_get_build_id");
    PFN_WINE_GET_HOST_VERSION pfnWineGetHostVersion =
        (PFN_WINE_GET_HOST_VERSION)GetProcAddress(hNtdll, "wine_get_host_version");

    // Distinguish CrossOver (CodeWeavers' commercial Wine) from plain Wine.
    // The build id of CrossOver contains the literal "CrossOver".
    bool bIsCrossOver = false;
    if(pfnWineGetBuildId != NULL)
    {
        const char* szBuildId = pfnWineGetBuildId();
        if(szBuildId != NULL && strstr(szBuildId, "CrossOver") != NULL)
            bIsCrossOver = true;
    }

    LPCTSTR szProduct = bIsCrossOver ? _T("CrossOver") : _T("Wine");

    sWineInfo = _T("emulated by ");
    sWineInfo += szProduct;

    const char* szVersion = pfnWineGetVersion();
    if(szVersion != NULL && *szVersion != '\0')
    {
        sWineInfo += _T(" ");
        sWineInfo += CString(szVersion);
    }

    // Resolve the host operating system (the OS Wine itself runs on).
    if(pfnWineGetHostVersion != NULL)
    {
        const char* szSysName = NULL;
        const char* szRelease = NULL;
        pfnWineGetHostVersion(&szSysName, &szRelease);

        if(szSysName != NULL && *szSysName != '\0')
        {
            // Map the kernel's sysname to a friendly host OS name.
            CString sHostOS;
            if(lstrcmpiA(szSysName, "Darwin") == 0)
                sHostOS = _T("macOS");
            else
                sHostOS = CString(szSysName); // e.g. "Linux", "FreeBSD", ...

            sWineInfo += _T(" on ");
            sWineInfo += sHostOS;

            // The release string is the host kernel version (for Darwin this is
            // the XNU kernel version, not the macOS marketing version).
            if(szRelease != NULL && *szRelease != '\0')
            {
                sWineInfo += _T(" (");
                sWineInfo += CString(szSysName);
                sWineInfo += _T(" ");
                sWineInfo += CString(szRelease);
                sWineInfo += _T(")");
            }

            // On macOS, also report the app bundle Wine is embedded in (see
            // GetWineAppBundleInfo); report explicitly if none was found.
            if(lstrcmpiA(szSysName, "Darwin") == 0)
            {
                CString sBundleInfo;
                sWineInfo += _T(", app bundle ");
                sWineInfo += (0 == GetWineAppBundleInfo(sBundleInfo)) ? sBundleInfo : CString(_T("not found"));
            }
        }
    }

    return 0;
}

BOOL Utility::IsOS64Bit()
{
    BOOL b64Bit = FALSE;

#ifdef _WIN64
    // 64-bit applications always run under 64-bit Windows
    return TRUE;
#endif

    // Check for 32-bit applications

    typedef BOOL (WINAPI *PFNISWOW64PROCESS)(HANDLE, PBOOL);

    HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
    if(hKernel32!=NULL)
    {
        PFNISWOW64PROCESS pfnIsWow64Process = 
            (PFNISWOW64PROCESS)GetProcAddress(hKernel32, "IsWow64Process");
        if(pfnIsWow64Process==NULL)
        {
            // If there is no IsWow64Process() API, than Windows is 32-bit for sure
            FreeLibrary(hKernel32);
            return FALSE;
        }

        pfnIsWow64Process(GetCurrentProcess(), &b64Bit);
        FreeLibrary(hKernel32);
    }

    return b64Bit;
}

int Utility::GetGeoLocation(CString& sGeoLocation)
{
    sGeoLocation = _T("");

    typedef GEOID (WINAPI *PFNGETUSERGEOID)(GEOCLASS);
    typedef int (WINAPI *PFNGETGEOINFOW)(GEOID, GEOTYPE, LPWSTR, int, LANGID);

    HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
    if(hKernel32!=NULL)
    {
        PFNGETUSERGEOID pfnGetUserGeoID = 
            (PFNGETUSERGEOID)GetProcAddress(hKernel32, "GetUserGeoID");
        PFNGETGEOINFOW pfnGetGeoInfoW = 
            (PFNGETGEOINFOW)GetProcAddress(hKernel32, "GetGeoInfoW");
        if(pfnGetUserGeoID==NULL || 
            pfnGetGeoInfoW==NULL)
            return -1;

        GEOID GeoLocation = pfnGetUserGeoID(GEOCLASS_NATION);
        if(GeoLocation!=GEOID_NOT_AVAILABLE)
        { 
            WCHAR szGeoInfo[1024] = _T("");    
            int n = pfnGetGeoInfoW(GeoLocation, GEO_RFC1766, szGeoInfo, 1024, 0);
            if(n!=0)
            {
                sGeoLocation = szGeoInfo;
                FreeLibrary(hKernel32);
                return 0;
            }
        }

        FreeLibrary(hKernel32);    
    }

    return -1;
}

int Utility::GetSpecialFolder(int csidl, CString& sFolderPath)
{
    sFolderPath.Empty();

    TCHAR szPath[_MAX_PATH];
    BOOL bResult = SHGetSpecialFolderPath(NULL, szPath, csidl, TRUE);
    if(!bResult)
        return 1;

    sFolderPath = CString(szPath);

    return 0;
}

CString Utility::ReplaceInvalidCharsInFileName(CString sFileName)
{
    sFileName.Replace(_T("*"),_T("_"));
    sFileName.Replace(_T("|"),_T("_"));
    sFileName.Replace(_T("/"),_T("_"));
    sFileName.Replace(_T("?"),_T("_"));
    sFileName.Replace(_T("<"),_T("_"));
    sFileName.Replace(_T(">"),_T("_"));
    return sFileName;
}

int Utility::RecycleFile(CString sFilePath, bool bPermanentDelete)
{
    SHFILEOPSTRUCT fop;
    memset(&fop, 0, sizeof(SHFILEOPSTRUCT));

    TCHAR szFrom[_MAX_PATH];  
    memset(szFrom, 0, sizeof(TCHAR)*(_MAX_PATH));
    _TCSCPY_S(szFrom, _MAX_PATH, sFilePath.GetBuffer(0));
    szFrom[sFilePath.GetLength()+1] = 0;

    fop.fFlags |= FOF_SILENT;                // don't report progress
    fop.fFlags |= FOF_NOERRORUI;           // don't report errors
    fop.fFlags |= FOF_NOCONFIRMATION;        // don't confirm delete
    fop.wFunc = FO_DELETE;                   // REQUIRED: delete operation
    fop.pFrom = szFrom;                      // REQUIRED: which file(s)
    fop.pTo = NULL;                          // MUST be NULL
    if (bPermanentDelete) 
    { 
        // if delete requested..
        fop.fFlags &= ~FOF_ALLOWUNDO;   // ..don't use Recycle Bin
    } 
    else 
    {                                 // otherwise..
        fop.fFlags |= FOF_ALLOWUNDO;    // ..send to Recycle Bin
    }

    return SHFileOperation(&fop); // do it!  
}

CString Utility::GetINIString(LPCTSTR pszFile, LPCTSTR pszSection, LPCTSTR pszName)
{  
    TCHAR szBuffer[1024] = _T("");  
    GetPrivateProfileString(pszSection, pszName, _T(""), szBuffer, 1024, pszFile);

    CString sResult = szBuffer;
    sResult.Replace(_T("\\n"), _T("\n"));

    return sResult;
}

void Utility::SetINIString(LPCTSTR pszFile, LPCTSTR pszSection, LPCTSTR pszName, LPCTSTR pszValue)
{   
    WritePrivateProfileString(pszSection, pszName, pszValue, pszFile);
}


void Utility::SetLayoutRTL(HWND hWnd)
{
    DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    dwExStyle |= WS_EX_LAYOUTRTL;
    SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);

    SetLayout(GetDC(hWnd), LAYOUT_RTL);

    CRect rcWnd;
    ::GetClientRect(hWnd, &rcWnd);

    HWND hWndChild = GetWindow(hWnd, GW_CHILD);
    while(hWndChild!=NULL)
    {    
        SetLayoutRTL(hWndChild);

        CRect rc;
        ::GetWindowRect(hWndChild, &rc);    
        ::MapWindowPoints(0, hWnd, (LPPOINT)&rc, 2);
        ::MoveWindow(hWndChild, rcWnd.Width()-rc.right, rc.top, rc.Width(), rc.Height(), TRUE);

        SetLayout(GetDC(hWndChild), LAYOUT_RTL);

        hWndChild = GetWindow(hWndChild, GW_HWNDNEXT);
    }  
}

CString Utility::FormatErrorMsg(DWORD dwErrorCode)
{
    LPTSTR msg = 0;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, dwErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&msg, 0, NULL);
    CString str = msg;
    str.Replace(_T("\r\n"), _T(""));
    GlobalFree(msg);
    return str;
}

// GetBaseFileName
// This helper function returns file name without extension
CString Utility::GetFileName(CString sPath)
{
    CString sBase = sPath;
    int pos1 = sPath.ReverseFind('\\');
    if(pos1>=0)
        sBase = sBase.Mid(pos1+1);

    return sBase;
}

// GetBaseFileName
// This helper function returns file name without extension
CString Utility::GetBaseFileName(CString sFileName)
{
    CString sBase = sFileName;
    int pos1 = sFileName.ReverseFind('\\');
    if(pos1>=0)
        sBase = sBase.Mid(pos1+1);

    int pos2 = sBase.ReverseFind('.');
    if(pos2>=0)
    {
        sBase = sFileName.Mid(0, pos2);
    }
    return sBase;
}

// GetFileExtension
// This helper function returns file extension by file name
CString Utility::GetFileExtension(CString sFileName)
{
    CString sExt;
    int pos = sFileName.ReverseFind('.');
    if(pos>=0)
    {
        sExt = sFileName.Mid(pos+1);
    }
    return sExt;
}

CString Utility::GetProductVersion(CString sModuleName)
{
    CString sProductVer; 

    DWORD dwBuffSize = GetFileVersionInfoSize(sModuleName, 0);
    LPBYTE pBuff = (LPBYTE)GlobalAlloc(GPTR, dwBuffSize);  

    if(NULL!=pBuff && 0!=GetFileVersionInfo(sModuleName, 0, dwBuffSize, pBuff))
    {
        VS_FIXEDFILEINFO* fi = NULL;
        UINT uLen = 0;
        VerQueryValue(pBuff, _T("\\"), (LPVOID*)&fi, &uLen);

        WORD dwVerMajor = HIWORD(fi->dwProductVersionMS);
        WORD dwVerMinor = LOWORD(fi->dwProductVersionMS);
        WORD dwPatchLevel = HIWORD(fi->dwProductVersionLS);
        WORD dwVerBuild = LOWORD(fi->dwProductVersionLS);

        sProductVer.Format(_T("%u.%u.%u.%u"), 
            dwVerMajor, dwVerMinor, dwPatchLevel, dwVerBuild);    
    } 

    GlobalFree((HGLOBAL)pBuff);

    return sProductVer;
}

// Creates a folder. If some intermediate folders in the path do not exist,
// it creates them.
BOOL Utility::CreateFolder(CString sFolderName)
{  
    CString sIntermediateFolder;

    // Skip disc drive name "X:\" if presents
    int start = sFolderName.Find(':', 0);
    if(start>=0)
        start+=2; 

    int pos = start;  
    for(;;)
    {
        pos = sFolderName.Find('\\', pos);
        if(pos<0)
        {
            sIntermediateFolder = sFolderName;
        }
        else
        {
            sIntermediateFolder = sFolderName.Left(pos);
        }

        BOOL bCreate = CreateDirectory(sIntermediateFolder, NULL);
        if(!bCreate && GetLastError()!=ERROR_ALREADY_EXISTS)
            return FALSE;

        DWORD dwAttrs = GetFileAttributes(sIntermediateFolder);
        if((dwAttrs&FILE_ATTRIBUTE_DIRECTORY)==0)
            return FALSE;

        if(pos==-1)
            break;

        pos++;
    }

    return TRUE;
}

ULONG64 Utility::SystemTimeToULONG64( const SYSTEMTIME& st )
{
    FILETIME ft ;
    SystemTimeToFileTime( &st, &ft ) ;
    ULARGE_INTEGER integer ;
    integer.LowPart = ft.dwLowDateTime ;
    integer.HighPart = ft.dwHighDateTime ;
    return integer.QuadPart ;
}

CString Utility::FileSizeToStr(ULONG64 uFileSize)
{
    CString sFileSize;

    if(uFileSize==0)
    {
        sFileSize = _T("0 KB");
    }
    else if(uFileSize<1024)
    {
        float fSizeKbytes = (float)uFileSize/(float)1024;
        TCHAR szStr[64];
#if _MSC_VER<1400
        _stprintf(szStr, _T("%0.1f KB"), fSizeKbytes);    
#else
        _stprintf_s(szStr, 64, _T("%0.1f KB"), fSizeKbytes);    
#endif
        sFileSize = szStr;
    }
    else if(uFileSize<1024*1024)
    {
        sFileSize.Format(_T("%I64u KB"), uFileSize/1024);
    }
    else
    {
        float fSizeMbytes = (float)uFileSize/(float)(1024*1024);
        TCHAR szStr[64];
#if _MSC_VER<1400
        _stprintf(szStr, _T("%0.1f MB"), fSizeMbytes);    
#else
        _stprintf_s(szStr, 64, _T("%0.1f MB"), fSizeMbytes);    
#endif
        sFileSize = szStr;
    }

    return sFileSize;
}

CString Utility::AddEllipsis(LPCTSTR szString, int nMaxLength)
{
	if(szString==NULL)
		return CString("");

	CString sResult = szString;
	if(sResult.GetLength()>nMaxLength)
	{
		if(nMaxLength>=3)
			sResult = sResult.Mid(0, nMaxLength-3)+_T("...");		
	}

	return sResult;
}

std::vector<CString> Utility::ExplodeStr(LPCTSTR szString, LPCTSTR szSeparators)
{
	std::vector<CString> aTokens;

	CString copy = szString;	
	TCHAR  *context = 0;
	TCHAR  *token = _tcstok_s(const_cast<LPTSTR>((LPCTSTR)copy), szSeparators, &context);	
	while (token != 0) 
	{
		aTokens.push_back(token);
		token=_tcstok_s(NULL, szSeparators, &context);		
	};

	return aTokens;
}

long Utility::GetFileSize(const TCHAR *fileName)
{
    BOOL                        fOk;
    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;

    if (NULL == fileName)
        return -1;

    fOk = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void*)&fileInfo);
    if (!fOk)
        return -1;
    //assert(0 == fileInfo.nFileSizeHigh);
    return (long)fileInfo.nFileSizeLow;
}

BOOL Utility::IsFileSearchPattern(CString sFileName)
{
	// Remove the "\\?\" prefix in case of a long path name
	if(sFileName.Left(4).Compare(_T("\\\\?\\"))==0)
		sFileName = sFileName.Mid(4);

	// Check if the file name is a search template.		
	BOOL bSearchPattern = FALSE;	
	int nPos = sFileName.FindOneOf(_T("*?"));
	if(nPos>=0)
		bSearchPattern = true;
	return bSearchPattern;
}

