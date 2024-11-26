#include "PlatformImplementation.h"

#include <platform/win/WinAPIShell.h>
#include <platform/win/RecursiveShellNotifier.h>
#include <platform/win/ThreadedQueueShellNotifier.h>

#include <QtPlatformHeaders/QWindowsWindowFunctions>

#include <Shlobj.h>
#include <Shlwapi.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <Aclapi.h>
#include <AccCtrl.h>
#include <comdef.h>
#include <wincred.h>
#include <taskschd.h>
#include <Sddl.h>
#include <time.h>
#include <iostream>
#include <dpapi.h>
#include <shellapi.h>

#include <QtWin>
#include <QOperatingSystemVersion>
#include <QScreen>
#include <QDesktopWidget>
#include <QHostInfo>

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#if _WIN32_WINNT < 0x0601
// Windows headers don't define this for WinXP despite the documentation says that they should
// and it indeed works
#ifndef SHFOLDERCUSTOMSETTINGS
#include <pshpack8.h>

// Used by SHGetSetFolderCustomSettings
typedef struct
{
    DWORD           dwSize;
    DWORD           dwMask;                 // IN/OUT  Which Attributes to Get/Set
    SHELLVIEWID*    pvid;                   // OUT - if dwReadWrite is FCS_READ, IN - otherwise
    // The folder's WebView template path
    LPWSTR          pszWebViewTemplate;     // OUT - if dwReadWrite is FCS_READ, IN - otherwise
    DWORD           cchWebViewTemplate;     // IN - Specifies the size of the buffer pointed to by pszWebViewTemplate
                                            // Ignored if dwReadWrite is FCS_READ
    LPWSTR           pszWebViewTemplateVersion;  // currently IN only
    // Infotip for the folder
    LPWSTR          pszInfoTip;             // OUT - if dwReadWrite is FCS_READ, IN - otherwise
    DWORD           cchInfoTip;             // IN - Specifies the size of the buffer pointed to by pszInfoTip
                                            // Ignored if dwReadWrite is FCS_READ
    // CLSID that points to more info in the registry
    CLSID*          pclsid;                 // OUT - if dwReadWrite is FCS_READ, IN - otherwise
    // Other flags for the folder. Takes FCS_FLAG_* values
    DWORD           dwFlags;                // OUT - if dwReadWrite is FCS_READ, IN - otherwise


    LPWSTR           pszIconFile;           // OUT - if dwReadWrite is FCS_READ, IN - otherwise
    DWORD            cchIconFile;           // IN - Specifies the size of the buffer pointed to by pszIconFile
                                            // Ignored if dwReadWrite is FCS_READ

    int              iIconIndex;            // OUT - if dwReadWrite is FCS_READ, IN - otherwise

    LPWSTR           pszLogo;               // OUT - if dwReadWrite is FCS_READ, IN - otherwise
    DWORD            cchLogo;               // IN - Specifies the size of the buffer pointed to by pszIconFile
                                            // Ignored if dwReadWrite is FCS_READ
} SHFOLDERCUSTOMSETTINGS, *LPSHFOLDERCUSTOMSETTINGS;

#include <poppack.h>        /* Return to byte packing */

// Gets/Sets the Folder Custom Settings for pszPath based on dwReadWrite. dwReadWrite can be FCS_READ/FCS_WRITE/FCS_FORCEWRITE
SHSTDAPI SHGetSetFolderCustomSettings(_Inout_ LPSHFOLDERCUSTOMSETTINGS pfcs, _In_ PCWSTR pszPath, DWORD dwReadWrite);
#endif
#endif

using namespace std;
using namespace mega;

bool WindowsPlatform_exiting = false;
static const QString NotAllowedDefaultFactoryBiosName = QString::fromUtf8("To be filled by O.E.M.");

void PlatformImplementation::initialize(int, char *[])
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    auto baseNotifier = std::make_shared<WindowsApiShellNotifier>();
    mSyncFileNotifier = std::make_shared<ThreadedQueueShellNotifier>(baseNotifier);
    mShellNotifier = std::make_shared<ThreadedQueueShellNotifier>(
                std::make_shared<RecursiveShellNotifier>(baseNotifier)
                );

    //In order to show dialogs when the application is inactive (for example, from the webclient)
    QWindowsWindowFunctions::setWindowActivationBehavior(QWindowsWindowFunctions::AlwaysActivateWindow);
}

void PlatformImplementation::prepareForSync()
{
    (void)Preferences::instance();

    QProcess p;
    p.start(QString::fromUtf8("net"), QStringList() << QString::fromUtf8("use"));
    p.waitForFinished(2000);
    QString output = QString::fromUtf8(p.readAllStandardOutput().constData());
    QString e = QString::fromUtf8(p.readAllStandardError().constData());
    if (e.size())
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error for \"net use\" command:");
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, e.toUtf8().constData());
    }

    QStringList data = output.split(QString::fromUtf8("\n"));
    if (data.size() > 1)
    {
        for (int i = 1; i < data.size(); i++)
        {
            QString drive = data.at(i).trimmed();
            if (drive.size() && drive.contains(QChar::fromLatin1(':')))
            {
                int index = drive.indexOf(QString::fromUtf8(":"));
                if (index >= 2 && drive[index - 2] == QChar::fromLatin1(' '))
                {
                    drive = drive.mid(index - 1);
                    QStringList parts = drive.split(QString::fromUtf8(" "), Qt::SkipEmptyParts);
                    if (parts.size() >= 2 && parts.at(1).startsWith(QString::fromUtf8("\\")))
                    {
                        QString driveName = parts.at(0);
                        QString networkName = parts.at(1);
                        MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Network drive detected: %1 (%2)")
                                    .arg(networkName, driveName).toUtf8().constData());

                        QStringList localFolders = SyncInfo::instance()->getLocalFolders(SyncInfo::AllHandledSyncTypes);
                        for (int i = 0; i < localFolders.size(); i++)
                        {
                            QString localFolder = localFolders.at(i);
                            MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Checking local synced folder: %1").arg(localFolder).toUtf8().constData());
                            if (localFolder.startsWith(driveName) || localFolder.startsWith(networkName))
                            {
                                MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Automatically mounting network drive")
                                             .toUtf8().constData());

                                QProcess p;
                                QString command = QString::fromUtf8("net");
                                p.start(command, QStringList() << QString::fromUtf8("use") << driveName << networkName);
                                p.waitForFinished(2000);
                                QString output = QString::fromUtf8(p.readAllStandardOutput().constData());
                                QString e = QString::fromUtf8(p.readAllStandardError().constData());
                                MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Output for \"%1\" command:").arg(command).toUtf8().constData());
                                MegaApi::log(MegaApi::LOG_LEVEL_INFO, output.toUtf8().constData());
                                if (e.size())
                                {
                                    MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Error for \"%1\" command:").arg(command).toUtf8().constData());
                                    MegaApi::log(MegaApi::LOG_LEVEL_ERROR, e.toUtf8().constData());
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

bool PlatformImplementation::enableTrayIcon(QString executable)
{
    HRESULT hr;
    ITrayNotify *m_ITrayNotify = nullptr;
    ITrayNotifyNew *m_ITrayNotifyNew = nullptr;

    hr = CoCreateInstance (
          __uuidof (TrayNotify),
          NULL,
          CLSCTX_LOCAL_SERVER,
          __uuidof (ITrayNotify),
          (PVOID *) &m_ITrayNotify);
    if (hr != S_OK)
    {
        hr = CoCreateInstance (
          __uuidof (TrayNotify),
          NULL,
          CLSCTX_LOCAL_SERVER,
          __uuidof (ITrayNotifyNew),
          (PVOID *) &m_ITrayNotifyNew);
        if (hr != S_OK)
        {
            return false;
        }
    }

    WinTrayReceiver *receiver = NULL;
    if (m_ITrayNotify)
    {
        receiver = new WinTrayReceiver(m_ITrayNotify, executable);
    }
    else
    {
        receiver = new WinTrayReceiver(m_ITrayNotifyNew, executable);
    }

    hr = receiver->start();
    if (hr != S_OK)
    {
        return false;
    }

    return true;
}

void PlatformImplementation::notifyItemChange(const QString& path, int)
{
    notifyItemChange(path, mShellNotifier);
}

void PlatformImplementation::notifySyncFileChange(std::string *localPath, int)
{
    QString path = QString::fromUtf8(localPath->c_str());
    notifyItemChange(path, mSyncFileNotifier);
}

void PlatformImplementation::notifyItemChange(const QString& localPath, std::shared_ptr<AbstractShellNotifier> notifier)
{
    if (!localPath.isEmpty())
    {
        notifier->notify(localPath);
    }
}

//From http://msdn.microsoft.com/en-us/library/windows/desktop/bb776891.aspx
HRESULT PlatformImplementation::CreateLink(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc,
                                 LPCWSTR pszIconfile, int iIconindex)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);
        if (pszIconfile)
        {
            psl->SetIconLocation(pszIconfile, iIconindex);
        }

        // Query IShellLink for the IPersistFile interface, used for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        {
            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

DWORD MEGARegDeleteKeyEx(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired, DWORD Reserved)
{
    typedef DWORD (WINAPI *RegDeleteKeyExWPtr)(HKEY hKey,
                                                     LPCWSTR lpSubKey,
                                                     REGSAM samDesired,
                                                     DWORD Reserved);

    RegDeleteKeyExWPtr RegDeleteKeyExWFunc;
    HMODULE hMod = GetModuleHandleW(L"advapi32.dll");
    if (!hMod || !(RegDeleteKeyExWFunc = (RegDeleteKeyExWPtr)GetProcAddress(hMod, "RegDeleteKeyExW")))
    {
        return RegDeleteKey(hKey, lpSubKey);
    }

    return RegDeleteKeyExWFunc(hKey, lpSubKey, samDesired, Reserved);
}

bool DeleteRegKey(HKEY key, LPTSTR subkey, REGSAM samDesired)
{
    TCHAR keyPath[MAX_PATH];
    DWORD maxSize;
    HKEY hKey;

    LONG result = MEGARegDeleteKeyEx(key, subkey, samDesired, 0);
    if (result == ERROR_SUCCESS)
    {
        return true;
    }

    result = RegOpenKeyEx(key, subkey, 0, samDesired | KEY_READ, &hKey);
    if (result != ERROR_SUCCESS)
    {
        return (result == ERROR_FILE_NOT_FOUND);
    }

    size_t len =  _tcslen(subkey);
    if (!len || len >= (MAX_PATH - 1) || _tcscpy_s(keyPath, MAX_PATH, subkey))
    {
        RegCloseKey(hKey);
        return false;
    }

    LPTSTR endPos = keyPath + len - 1;
    if (*(endPos++) != TEXT('\\'))
    {
        *(endPos++) =  TEXT('\\');
        *endPos =  TEXT('\0');
        len++;
    }

    do
    {
        maxSize = DWORD(MAX_PATH - len);
    } while (RegEnumKeyEx(hKey, 0, endPos, &maxSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS
             && DeleteRegKey(key, keyPath, samDesired));

    RegCloseKey(hKey);
    return (MEGARegDeleteKeyEx(key, subkey, samDesired, 0) == ERROR_SUCCESS);
}

bool DeleteRegValue(HKEY key, LPTSTR subkey, LPTSTR value, REGSAM samDesired)
{
    HKEY hKey;
    LONG result = RegOpenKeyEx(key, subkey, 0, samDesired | KEY_SET_VALUE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        return (result == ERROR_FILE_NOT_FOUND);
    }

    result = RegDeleteValue(hKey, value);
    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS);
}

bool SetRegistryKeyAndValue(HKEY hkey, PCWSTR subkey, DWORD dwType,
                            PCWSTR valuename, const BYTE *pszData, DWORD cbData,
                            REGSAM samDesired)
{
    HRESULT hr;
    HKEY hKey = NULL;
    hr = HRESULT_FROM_WIN32(RegCreateKeyEx(hkey, subkey, 0, NULL,
                                           REG_OPTION_NON_VOLATILE,
                                           KEY_WRITE | samDesired,
                                           NULL, &hKey, NULL));
    if (!SUCCEEDED(hr))
    {
        return false;
    }

    hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, valuename, 0, dwType, pszData, cbData));
    RegCloseKey(hKey);
    return SUCCEEDED(hr);
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL IsWow64()
{
    BOOL bIsWow64 = TRUE;
    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
    if (fnIsWow64Process)
    {
        fnIsWow64Process(GetCurrentProcess(),&bIsWow64);
    }
    return bIsWow64;
}

bool CheckLeftPaneIcon(wchar_t *path, bool remove)
{
    HKEY hKey = NULL;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
                               L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace",
                               0, KEY_READ | KEY_ENUMERATE_SUB_KEYS, &hKey);
    if (result != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD retCode = ERROR_SUCCESS;
    WCHAR uuid[MAX_PATH];
    DWORD uuidlen = sizeof(uuid);

    for (int i = 0; retCode == ERROR_SUCCESS; i++)
    {
        uuidlen = sizeof(uuid);
        uuid[0] = '\0';
        retCode = RegEnumKeyEx(hKey, i, uuid, &uuidlen, NULL, NULL, NULL, NULL);
        if (retCode == ERROR_SUCCESS)
        {
            HKEY hSubKey;
            TCHAR subKeyPath[MAX_PATH];
            swprintf_s(subKeyPath, MAX_PATH, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\%s", uuid);
            result = RegOpenKeyEx(HKEY_CURRENT_USER, subKeyPath, 0, KEY_READ, &hSubKey);
            if (result != ERROR_SUCCESS)
            {
                continue;
            }

            DWORD type;
            TCHAR value[MAX_PATH];
            DWORD valuelen = sizeof(value);
            result = RegQueryValueEx(hSubKey, L"", NULL, &type, (LPBYTE)value, &valuelen);
            RegCloseKey(hSubKey);
            if (result != ERROR_SUCCESS || type != REG_SZ || valuelen != 10 || memcmp(value, L"MEGA", 10))
            {
                continue;
            }

            if (path)
            {
                bool found = false;

                swprintf_s(subKeyPath, MAX_PATH, L"Software\\Classes\\CLSID\\%s\\Instance\\InitPropertyBag", uuid);
                result = RegOpenKeyEx(HKEY_CURRENT_USER, subKeyPath, 0, KEY_READ, &hSubKey);
                if (result == ERROR_SUCCESS)
                {
                    valuelen = sizeof(value);
                    result = RegQueryValueEx(hSubKey, L"TargetFolderPath", NULL, &type, (LPBYTE)value, &valuelen);
                    RegCloseKey(hSubKey);
                    if (result == ERROR_SUCCESS && type == REG_EXPAND_SZ && !_wcsicmp(value, path))
                    {
                        found = true;
                    }
                }

                if (!found)
                {
                    swprintf_s(subKeyPath, MAX_PATH, L"Software\\Classes\\CLSID\\%s\\Instance\\InitPropertyBag", uuid);
                    result = RegOpenKeyEx(HKEY_CURRENT_USER, subKeyPath, 0, KEY_WOW64_64KEY | KEY_READ, &hSubKey);
                    if (result == ERROR_SUCCESS)
                    {
                        valuelen = sizeof(value);
                        result = RegQueryValueEx(hSubKey, L"TargetFolderPath", NULL, &type, (LPBYTE)value, &valuelen);
                        RegCloseKey(hSubKey);
                        if (result == ERROR_SUCCESS && type == REG_EXPAND_SZ && !_wcsicmp(value, path))
                        {
                            found = true;
                        }
                    }
                }

                if (!found)
                {
                    continue;
                }
            }

            if (remove)
            {
                wchar_t buffer[MAX_PATH];
                swprintf_s(buffer, MAX_PATH, L"Software\\Classes\\CLSID\\%s", uuid);

                if (IsWow64())
                {
                    DeleteRegKey(HKEY_CURRENT_USER, buffer, KEY_WOW64_64KEY);
                }
                DeleteRegKey(HKEY_CURRENT_USER, buffer, 0);

                DeleteRegValue(HKEY_CURRENT_USER,
                               (LPTSTR)L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel",
                               uuid, 0);
                swprintf_s(buffer, MAX_PATH, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\%s", uuid);
                DeleteRegKey(HKEY_CURRENT_USER, buffer, 0);
            }
            RegCloseKey(hKey);
            return true;
        }
    }
    RegCloseKey(hKey);
    return false;
}

void PlatformImplementation::addSyncToLeftPane(QString syncPath, QString syncName, QString uuid)
{
    typedef LONG MEGANTSTATUS;
    typedef struct _MEGAOSVERSIONINFOW {
        DWORD dwOSVersionInfoSize;
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        DWORD dwBuildNumber;
        DWORD dwPlatformId;
        WCHAR  szCSDVersion[ 128 ];     // Maintenance string for PSS usage
    } MEGARTL_OSVERSIONINFOW, *PMEGARTL_OSVERSIONINFOW;

    typedef MEGANTSTATUS (WINAPI* RtlGetVersionPtr)(PMEGARTL_OSVERSIONINFOW);
    MEGARTL_OSVERSIONINFOW version = { 0 };
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (!hMod)
    {
        return;
    }

    RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
    if (!RtlGetVersion)
    {
        return;
    }

    RtlGetVersion(&version);
    if (version.dwMajorVersion < 10)
    {
        return;
    }

    DWORD value;
    QString sValue;
    QString key = QString::fromUtf8("Software\\Classes\\CLSID\\%1").arg(uuid);
    REGSAM samDesired = 0;
    int it = 1;

    if (IsWow64())
    {
        samDesired = KEY_WOW64_64KEY;
        it++;
    }

    for (int i = 0; i < it; i++)
    {
        sValue = syncName;
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)key.utf16(),
                               REG_SZ, L"", (const BYTE *)sValue.utf16(), sValue.size() * 2,
                               samDesired);

        sValue = MegaApplication::applicationFilePath();
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)(key + QString::fromUtf8("\\DefaultIcon")).utf16(),
                               REG_EXPAND_SZ, L"", (const BYTE *)sValue.utf16(), sValue.size() * 2,
                               samDesired);

        value = 0x1;
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)key.utf16(),
                               REG_DWORD, L"System.IsPinnedToNameSpaceTree", (const BYTE *)&value, sizeof(DWORD),
                               samDesired);

        value = 0x42;
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)key.utf16(),
                               REG_DWORD, L"SortOrderIndex", (const BYTE *)&value, sizeof(DWORD),
                               samDesired);

        sValue = QString::fromUtf8("%systemroot%\\system32\\shell32.dll");
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)(key + QString::fromUtf8("\\InProcServer32")).utf16(),
                               REG_EXPAND_SZ, L"", (const BYTE *)sValue.utf16(), sValue.size() * 2,
                               samDesired);

        sValue = QString::fromUtf8("{0E5AAE11-A475-4c5b-AB00-C66DE400274E}");
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)(key + QString::fromUtf8("\\Instance")).utf16(),
                               REG_SZ, L"CLSID", (const BYTE *)sValue.utf16(), sValue.size() * 2,
                               samDesired);

        value = 0x10;
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)(key + QString::fromUtf8("\\Instance\\InitPropertyBag")).utf16(),
                               REG_DWORD, L"Attributes", (const BYTE *)&value, sizeof(DWORD),
                               samDesired);

        sValue = QDir::toNativeSeparators(syncPath);
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)(key + QString::fromUtf8("\\Instance\\InitPropertyBag")).utf16(),
                               REG_EXPAND_SZ, L"TargetFolderPath", (const BYTE *)sValue.utf16(), sValue.size() * 2,
                               samDesired);

        value = 0x28;
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)(key + QString::fromUtf8("\\ShellFolder")).utf16(),
                               REG_DWORD, L"FolderValueFlags", (const BYTE *)&value, sizeof(DWORD),
                               samDesired);

        value = 0xF080004D;
        SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR)(key + QString::fromUtf8("\\ShellFolder")).utf16(),
                               REG_DWORD, L"Attributes", (const BYTE *)&value, sizeof(DWORD),
                               samDesired);

        samDesired = 0;
    }

    sValue = QString::fromUtf8("MEGA");
    SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR) QString::fromUtf8("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\%1").arg(uuid).utf16(),
                           REG_SZ, L"", (const BYTE *)sValue.utf16(), sValue.size() * 2,
                           samDesired);

    value = 0x1;
    SetRegistryKeyAndValue(HKEY_CURRENT_USER, (LPTSTR) QString::fromUtf8("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel").utf16(),
                           REG_DWORD, (LPTSTR)uuid.utf16(), (const BYTE *)&value, sizeof(DWORD),
                           samDesired);
}

void PlatformImplementation::removeSyncFromLeftPane(QString, QString, QString uuid)
{
    REGSAM samDesired = 0;
    int it = 1;

    if (IsWow64())
    {
        samDesired = KEY_WOW64_64KEY;
        it++;
    }

    QString key = QString::fromUtf8("Software\\Classes\\CLSID\\%1").arg(uuid);
    for (int i = 0; i < it; i++)
    {
        DeleteRegKey(HKEY_CURRENT_USER, (LPTSTR)key.utf16(), samDesired);
        samDesired = 0;
    }

    key = QString::fromUtf8("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Desktop\\NameSpace\\%1").arg(uuid);
    DeleteRegKey(HKEY_CURRENT_USER, (LPTSTR)key.utf16(), 0);

    key = QString::fromUtf8("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel");
    DeleteRegValue(HKEY_CURRENT_USER, (LPTSTR)key.utf16(), (LPTSTR)uuid.utf16(), 0);
}

void PlatformImplementation::removeAllSyncsFromLeftPane()
{
    bool deleted = true;
    while (deleted)
    {
        deleted &= CheckLeftPaneIcon(NULL, true);
    }
}

bool PlatformImplementation::makePubliclyReadable(const QString& fileName)
{
    auto winFilename = (LPTSTR)fileName.utf16();
    bool result = false;
    PACL pOldDACL = NULL, pNewDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD sidSize = SECURITY_MAX_SID_SIZE;
    EXPLICIT_ACCESS ea;

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    if (winFilename
            && (GetNamedSecurityInfo(winFilename, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD) == ERROR_SUCCESS)
            && (ea.Trustee.ptstrName = (LPWSTR)LocalAlloc(LMEM_FIXED, sidSize))
            && CreateWellKnownSid(WinBuiltinUsersSid, NULL, ea.Trustee.ptstrName, &sidSize)
            && (SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL) == ERROR_SUCCESS)
            && (SetNamedSecurityInfo(winFilename, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDACL, NULL) == ERROR_SUCCESS))
    {
        result = true;
    }

    if (ea.Trustee.ptstrName != NULL)
    {
        LocalFree(ea.Trustee.ptstrName);
    }
    if(pSD != NULL)
    {
        LocalFree((HLOCAL) pSD);
    }
    if(pNewDACL != NULL)
    {
        LocalFree((HLOCAL) pNewDACL);
    }
    return result;
}

void PlatformImplementation::streamWithApp(const QString &app, const QString &url)
{
    if (isTraditionalApp(app))
    {
        QProcess::startDetached(app, QStringList{ url });
    }
    else
    {
        QString command = buildWindowsModernAppCommand(app);
        auto fullCommand = QString::fromLatin1("cmd.exe /c start \"%1\" %2").arg(command, url);
        std::system(fullCommand.toUtf8().constData()); // QProcess::start doesn't work
    }
}

bool PlatformImplementation::startOnStartup(bool value)
{
    WCHAR path[MAX_PATH];
    HRESULT res = SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, path);
    if (res != S_OK)
    {
        return false;
    }

    QString startupPath = QString::fromWCharArray(path);
    startupPath += QString::fromLatin1("\\MEGAsync.lnk");

    if (value)
    {
        if (QFile(startupPath).exists())
        {
            return true;
        }

        WCHAR wDescription[]=L"Start MEGAsync";
        WCHAR *wStartupPath = (WCHAR *)startupPath.utf16();

        QString exec = MegaApplication::applicationFilePath();
        exec = QDir::toNativeSeparators(exec);
        WCHAR *wExecPath = (WCHAR *)exec.utf16();

        res = CreateLink(wExecPath, wStartupPath, wDescription);

        if (res != S_OK)
        {
            return false;
        }
        return true;
    }
    else
    {
        QFile::remove(startupPath);
        return true;
    }
}

bool PlatformImplementation::isStartOnStartupActive()
{
    WCHAR path[MAX_PATH];
    HRESULT res = SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, path);
    if (res != S_OK)
    {
        return false;
    }

    QString startupPath = QString::fromWCharArray(path);
    startupPath += QString::fromLatin1("\\MEGAsync.lnk");
    if (QFileInfo(startupPath).isSymLink())
    {
        return true;
    }
    return false;
}

bool PlatformImplementation::showInFolder(QString pathIn)
{
    if (!QFile(pathIn).exists())
    {
        return false;
    }

    return QProcess::startDetached(QString::fromLatin1("explorer.exe"), QStringList{QString::fromLatin1("/select"),
                                                                                    QString::fromLatin1(","),
                                                                                    QDir::toNativeSeparators(QDir(pathIn).canonicalPath())});
}

void PlatformImplementation::startShellDispatcher(MegaApplication *receiver)
{
    if (shellDispatcherTask)
    {
        return;
    }
    shellDispatcherTask = new WinShellDispatcherTask(receiver);
    shellDispatcherTask->start();
}

void PlatformImplementation::stopShellDispatcher()
{
    if (shellDispatcherTask)
    {
        shellDispatcherTask->exitTask();
        shellDispatcherTask->wait();
        delete shellDispatcherTask;
        shellDispatcherTask = NULL;
    }
}

void PlatformImplementation::syncFolderAdded(QString syncPath, QString syncName, QString syncID)
{
    if (syncPath.startsWith(QString::fromLatin1("\\\\?\\")))
    {
        syncPath = syncPath.mid(4);
    }

    if (!syncPath.size())
    {
        return;
    }

    QDir syncDir(syncPath);
    if (!syncDir.exists())
    {
        return;
    }

    if (!Preferences::instance()->leftPaneIconsDisabled())
    {
        addSyncToLeftPane(syncPath, syncName, syncID);
    }

    int iconIndex = 2; // Default to winXP.ico

    auto winVersion = QOperatingSystemVersion::current();
    auto win11 = QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 22000);
    //FIXME: once QT version update check if QOperatingSystemVersion Windows11 exist
    if (winVersion >= win11)
    {
        iconIndex = 4; // win11.ico
    }
    else if (winVersion >= QOperatingSystemVersion::Windows7)
    {
        iconIndex = 3; // win8.ico
    }


    QString infoTip = QCoreApplication::translate("WindowsPlatform", "MEGA synced folder");
    SHFOLDERCUSTOMSETTINGS fcs = {0};
    fcs.dwSize = sizeof(SHFOLDERCUSTOMSETTINGS);
    fcs.dwMask = FCSM_ICONFILE | FCSM_INFOTIP;
    fcs.pszIconFile = (LPWSTR)MegaApplication::applicationFilePath().utf16();
    fcs.iIconIndex = iconIndex;
    fcs.pszInfoTip = (LPWSTR)infoTip.utf16();
    SHGetSetFolderCustomSettings(&fcs, (LPCWSTR)syncPath.utf16(), FCS_FORCEWRITE);

    WCHAR path[MAX_PATH];
    HRESULT res = SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path);
    if (res != S_OK)
    {
        return;
    }

    QString linksPath = QString::fromWCharArray(path);
    linksPath += QString::fromLatin1("\\Links");
    QFileInfo info(linksPath);
    if (!info.isDir())
    {
        return;
    }

    QString linkPath = linksPath + QString::fromLatin1("\\") + syncName + QString::fromLatin1(".lnk");
    if (QFile(linkPath).exists())
    {
        return;
    }

    WCHAR wDescription[]=L"MEGAsync synchronized folder";
    linkPath = QDir::toNativeSeparators(linkPath);
    WCHAR *wLinkPath = (WCHAR *)linkPath.utf16();

    syncPath = QDir::toNativeSeparators(syncPath);
    WCHAR *wSyncPath = (WCHAR *)syncPath.utf16();

    QString exec = MegaApplication::applicationFilePath();
    exec = QDir::toNativeSeparators(exec);
    WCHAR *wExecPath = (WCHAR *)exec.utf16();
    res = CreateLink(wSyncPath, wLinkPath, wDescription, wExecPath);

    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH | SHCNF_FLUSHNOWAIT, wLinkPath, NULL);

    WCHAR *wLinksPath = (WCHAR *)linksPath.utf16();
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSHNOWAIT, wLinksPath, NULL);
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, syncPath.utf16(), NULL);

    //Hide debris folder
    QString debrisPath = QDir::toNativeSeparators(syncPath + QDir::separator() + QString::fromLatin1(MEGA_DEBRIS_FOLDER));
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW((LPCWSTR)debrisPath.utf16(), GetFileExInfoStandard, &fad))
    {
        SetFileAttributesW((LPCWSTR)debrisPath.utf16(), fad.dwFileAttributes | FILE_ATTRIBUTE_HIDDEN);
    }
}

void PlatformImplementation::syncFolderRemoved(QString syncPath, QString syncName, QString syncID)
{
    if (!syncPath.size())
    {
        return;
    }

    removeSyncFromLeftPane(syncPath, syncName, syncID);

    if (syncPath.startsWith(QString::fromLatin1("\\\\?\\")))
    {
        syncPath = syncPath.mid(4);
    }

    SHFOLDERCUSTOMSETTINGS fcs = {0};
    fcs.dwSize = sizeof(SHFOLDERCUSTOMSETTINGS);
    fcs.dwMask = FCSM_ICONFILE | FCSM_INFOTIP;
    SHGetSetFolderCustomSettings(&fcs, (LPCWSTR)syncPath.utf16(), FCS_FORCEWRITE);

    WCHAR path[MAX_PATH];
    HRESULT res = SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path);
    if (res != S_OK)
    {
        return;
    }

    QString linksPath = QString::fromWCharArray(path);
    linksPath += QString::fromLatin1("\\Links");
    QFileInfo info(linksPath);
    if (!info.isDir())
    {
        return;
    }

    QString linkPath = linksPath + QString::fromLatin1("\\") + syncName + QString::fromLatin1(".lnk");

    QFile::remove(linkPath);

    WCHAR *wLinkPath = (WCHAR *)linkPath.utf16();
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH | SHCNF_FLUSHNOWAIT, wLinkPath, NULL);

    WCHAR *wLinksPath = (WCHAR *)linksPath.utf16();
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSHNOWAIT, wLinksPath, NULL);
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT, syncPath.utf16(), NULL);
}

void PlatformImplementation::notifyRestartSyncFolders()
{

}

void PlatformImplementation::notifyAllSyncFoldersAdded()
{

}

void PlatformImplementation::notifyAllSyncFoldersRemoved()
{

}

void PlatformImplementation::processSymLinks()
{

}

QByteArray PlatformImplementation::encrypt(QByteArray data, QByteArray key)
{
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
    DATA_BLOB entropy;

    dataIn.pbData = (BYTE *)data.constData();
    dataIn.cbData = data.size();
    entropy.pbData = (BYTE *)key.constData();
    entropy.cbData = key.size();

    if (!CryptProtectData(&dataIn, L"", &entropy, NULL, NULL, 0, &dataOut))
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error encrypting data: %1. data.size = %2, key.size = %3")
                     .arg(GetLastError()).arg(data.size()).arg(key.size()).toUtf8().constData());
        return data;
    }

    QByteArray result((const char *)dataOut.pbData, dataOut.cbData);
    LocalFree(dataOut.pbData);
    return result;
}

QByteArray PlatformImplementation::decrypt(QByteArray data, QByteArray key)
{
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;
    DATA_BLOB entropy;

    dataIn.pbData = (BYTE *)data.constData();
    dataIn.cbData = data.size();
    entropy.pbData = (BYTE *)key.constData();
    entropy.cbData = key.size();

    if (!CryptUnprotectData(&dataIn, NULL, &entropy, NULL, NULL, 0, &dataOut))
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error decrypting data: %1. data.size = %2, key.size = %3")
                     .arg(GetLastError()).arg(data.size()).arg(key.size()).toUtf8().constData());
        return data;
    }

    QByteArray result((const char *)dataOut.pbData, dataOut.cbData);
    LocalFree(dataOut.pbData);
    return result;
}

QByteArray PlatformImplementation::getLocalStorageKey()
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error getting Local Storage key. At OpenProcessToken: %1")
                     .arg(GetLastError()).toUtf8().constData());
        return QByteArray();
    }

    DWORD dwBufferSize = 0;
    auto r = GetTokenInformation(hToken, TokenUser, NULL, 0, &dwBufferSize); //Note: return value can be ERROR_INSUFFICIENT_BUFFER in an otherwise succesful call (just getting size)
    if ((!r && GetLastError() != ERROR_INSUFFICIENT_BUFFER) || !dwBufferSize)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error getting Local Storage key. At GetTokenInformation size retrieval: %1")
                     .arg(GetLastError()).toUtf8().constData());
        CloseHandle(hToken);
        return QByteArray();
    }

    PTOKEN_USER userToken = (PTOKEN_USER)new char[dwBufferSize];
    if (!GetTokenInformation(hToken, TokenUser, userToken, dwBufferSize, &dwBufferSize))
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error getting Local Storage key. At GetTokenInformation: %1")
                     .arg(GetLastError()).toUtf8().constData());
        CloseHandle(hToken);
        delete userToken;
        return QByteArray();
    }

    if (!IsValidSid(userToken->User.Sid))
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error getting Local Storage key. At IsValidSid: %1")
                     .arg(GetLastError()).toUtf8().constData());
        CloseHandle(hToken);
        delete userToken;
        return QByteArray();
    }

    DWORD dwLength = GetLengthSid(userToken->User.Sid);

    MegaApi::log(MegaApi::LOG_LEVEL_DEBUG, QString::fromUtf8("Getting Local Storage key. Sid length: %1")
                 .arg(dwLength).toUtf8().constData());

    QByteArray result((char *)userToken->User.Sid, dwLength);
    CloseHandle(hToken);
    delete userToken;
    return result;
}

QString PlatformImplementation::getDefaultOpenApp(QString extension)
{
    const QString extensionWithDot = QString::fromUtf8(".") + extension;

    const QString mimeType = findMimeType(extensionWithDot);
    if (!mimeType.startsWith(QString::fromUtf8("video")))
    {
        return QString();
    }

    const QString executableApp = findAssociatedExecutable(extensionWithDot);
    if (!executableApp.isEmpty())
    {
        return executableApp;
    }

    // SNC-3353
    // Films and TV app doesn't support streaming. In case this is the detected app,
    // Detecting it here will cause its call to use the browser for download instead.
    // Current behaviour is to fall back to Windows Media player Legacy, which is a
    // better option.
    // In case it is necessary to start detecting Windows modern app again,
    // just uncomment this code and it should work.
    /*
    const QString modernApp = findAssociatedModernApp(extensionWithDot);
    if (!modernApp.isEmpty())
    {
        return modernApp;
    }*/

    return getDefaultVideoPlayer();
}

void PlatformImplementation::enableDialogBlur(QDialog*)
{
    // this feature doesn't work well yet
    return;

#if 0
    bool win10 = false;
    HWND hWnd = (HWND)dialog->winId();
    dialog->setAttribute(Qt::WA_TranslucentBackground, true);
    dialog->setAttribute(Qt::WA_NoSystemBackground, true);

    const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
    if (hModule)
    {
        struct ACCENTPOLICY
        {
            int nAccentState;
            int nFlags;
            int nColor;
            int nAnimationId;
        };
        struct WINCOMPATTRDATA
        {
            int nAttribute;
            PVOID pData;
            ULONG ulDataSize;
        };
        typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute)
        {
            ACCENTPOLICY policy = { 3, 0, int(0xFFFFFFFF), 0 };
            WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hWnd, &data);
            win10 = true;
        }
        FreeLibrary(hModule);
    }

    if (!win10)
    {
        QtWin::setCompositionEnabled(true);
        QtWin::extendFrameIntoClientArea(dialog, -1, -1, -1, -1);
        QtWin::enableBlurBehindWindow(dialog);
    }
#endif
}

LPTSTR PlatformImplementation::getCurrentSid()
{
    HANDLE hTok = NULL;
    LPBYTE buf = NULL;
    DWORD  dwSize = 0;
    LPTSTR stringSID = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok))
    {
        GetTokenInformation(hTok, TokenUser, NULL, 0, &dwSize);
        if (dwSize)
        {
            buf = (LPBYTE)LocalAlloc(LPTR, dwSize);
            if (GetTokenInformation(hTok, TokenUser, buf, dwSize, &dwSize))
            {
                ConvertSidToStringSid(((PTOKEN_USER)buf)->User.Sid, &stringSID);
            }
            LocalFree(buf);
        }
        CloseHandle(hTok);
    }
    return stringSID;
}

bool PlatformImplementation::registerUpdateJob()
{
    ITaskService *pService = NULL;
    ITaskFolder *pRootFolder = NULL;
    ITaskFolder *pMEGAFolder = NULL;
    ITaskDefinition *pTask = NULL;
    IRegistrationInfo *pRegInfo = NULL;
    IPrincipal *pPrincipal = NULL;
    ITaskSettings *pSettings = NULL;
    IIdleSettings *pIdleSettings = NULL;
    ITriggerCollection *pTriggerCollection = NULL;
    ITrigger *pTrigger = NULL;
    IDailyTrigger *pCalendarTrigger = NULL;
    IRepetitionPattern *pRepetitionPattern = NULL;
    IActionCollection *pActionCollection = NULL;
    IAction *pAction = NULL;
    IExecAction *pExecAction = NULL;
    IRegisteredTask *pRegisteredTask = NULL;
    time_t currentTime;
    struct tm* currentTimeInfo;
    WCHAR currentTimeString[128];
    _bstr_t taskBaseName = L"MEGAsync Update Task ";
    LPTSTR stringSID = NULL;
    bool success = false;

    stringSID = getCurrentSid();
    if (!stringSID)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Unable to get the current SID");
        return false;
    }

    time(&currentTime);
    currentTimeInfo = localtime(&currentTime);
    wcsftime(currentTimeString, 128,  L"%Y-%m-%dT%H:%M:%S", currentTimeInfo);
    _bstr_t taskName = taskBaseName + stringSID;
    _bstr_t userId = stringSID;
    LocalFree(stringSID);
    QString MEGAupdaterPath = QDir::toNativeSeparators(QDir(MegaApplication::applicationDirPath()).filePath(QString::fromUtf8("MEGAupdater.exe")));

    HRESULT initializeSecurityResult = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);

    if ( (SUCCEEDED(initializeSecurityResult) || initializeSecurityResult == RPC_E_TOO_LATE /* already called */ )
            && SUCCEEDED(CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService))
            && SUCCEEDED(pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()))
            && SUCCEEDED(pService->GetFolder(_bstr_t( L"\\"), &pRootFolder)))
    {
        if (pRootFolder->CreateFolder(_bstr_t(L"MEGA"), _variant_t(L""), &pMEGAFolder) == 0x800700b7)
        {
            pRootFolder->GetFolder(_bstr_t(L"MEGA"), &pMEGAFolder);
        }

        if (pMEGAFolder
                && SUCCEEDED(pService->NewTask(0, &pTask))
                && SUCCEEDED(pTask->get_RegistrationInfo(&pRegInfo))
                && SUCCEEDED(pRegInfo->put_Author(_bstr_t(L"MEGA Limited")))
                && SUCCEEDED(pTask->get_Principal(&pPrincipal))
                && SUCCEEDED(pPrincipal->put_Id(_bstr_t(L"Principal1")))
                && SUCCEEDED(pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN))
                && SUCCEEDED(pPrincipal->put_RunLevel(TASK_RUNLEVEL_LUA))
                && SUCCEEDED(pPrincipal->put_UserId(userId))
                && SUCCEEDED(pTask->get_Settings(&pSettings))
                && SUCCEEDED(pSettings->put_StartWhenAvailable(VARIANT_TRUE))
                && SUCCEEDED(pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE))
                && SUCCEEDED(pSettings->get_IdleSettings(&pIdleSettings))
                && SUCCEEDED(pIdleSettings->put_StopOnIdleEnd(VARIANT_FALSE))
                && SUCCEEDED(pIdleSettings->put_RestartOnIdle(VARIANT_FALSE))
                && SUCCEEDED(pIdleSettings->put_WaitTimeout(_bstr_t()))
                && SUCCEEDED(pIdleSettings->put_IdleDuration(_bstr_t()))
                && SUCCEEDED(pTask->get_Triggers(&pTriggerCollection))
                && SUCCEEDED(pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pTrigger))
                && SUCCEEDED(pTrigger->QueryInterface(IID_IDailyTrigger, (void**) &pCalendarTrigger))
                && SUCCEEDED(pCalendarTrigger->put_Id(_bstr_t(L"Trigger1")))
                && SUCCEEDED(pCalendarTrigger->put_DaysInterval(1))
                && SUCCEEDED(pCalendarTrigger->put_StartBoundary(_bstr_t(currentTimeString)))
                && SUCCEEDED(pCalendarTrigger->get_Repetition(&pRepetitionPattern))
                && SUCCEEDED(pRepetitionPattern->put_Duration(_bstr_t(L"P1D")))
                && SUCCEEDED(pRepetitionPattern->put_Interval(_bstr_t(L"PT2H")))
                && SUCCEEDED(pRepetitionPattern->put_StopAtDurationEnd(VARIANT_FALSE))
                && SUCCEEDED(pTask->get_Actions(&pActionCollection))
                && SUCCEEDED(pActionCollection->Create(TASK_ACTION_EXEC, &pAction))
                && SUCCEEDED(pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction))
                && SUCCEEDED(pExecAction->put_Path(_bstr_t((wchar_t *)MEGAupdaterPath.utf16()))))
        {
            if (SUCCEEDED(pMEGAFolder->RegisterTaskDefinition(taskName, pTask,
                    TASK_CREATE_OR_UPDATE, _variant_t(), _variant_t(),
                    TASK_LOGON_INTERACTIVE_TOKEN, _variant_t(L""),
                    &pRegisteredTask)))
            {
                success = true;
                MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Update task registered OK");
            }
            else
            {
                MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error registering update task");
            }
        }
        else
        {
            MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error creating update task");
        }
    }
    else
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error getting root task folder");
    }

    if (pRegisteredTask)
    {
        pRegisteredTask->Release();
    }
    if (pTrigger)
    {
        pTrigger->Release();
    }
    if (pTriggerCollection)
    {
        pTriggerCollection->Release();
    }
    if (pIdleSettings)
    {
        pIdleSettings->Release();
    }
    if (pSettings)
    {
        pSettings->Release();
    }
    if (pPrincipal)
    {
        pPrincipal->Release();
    }
    if (pRegInfo)
    {
        pRegInfo->Release();
    }
    if (pCalendarTrigger)
    {
        pCalendarTrigger->Release();
    }
    if (pAction)
    {
        pAction->Release();
    }
    if (pActionCollection)
    {
        pActionCollection->Release();
    }
    if (pRepetitionPattern)
    {
        pRepetitionPattern->Release();
    }
    if (pExecAction)
    {
        pExecAction->Release();
    }
    if (pTask)
    {
        pTask->Release();
    }
    if (pMEGAFolder)
    {
        pMEGAFolder->Release();
    }
    if (pRootFolder)
    {
        pRootFolder->Release();
    }
    if (pService)
    {
        pService->Release();
    }

    return success;
}

void PlatformImplementation::uninstall()
{
    removeAllSyncsFromLeftPane();

    ITaskService *pService = NULL;
    ITaskFolder *pRootFolder = NULL;
    ITaskFolder *pMEGAFolder = NULL;
    _bstr_t taskBaseName = L"MEGAsync Update Task ";
    LPTSTR stringSID = NULL;

    stringSID = getCurrentSid();
    if (!stringSID)
    {
        return;
    }
    _bstr_t taskName = taskBaseName + stringSID;


    HRESULT initializeSecurityResult = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);

    if (initializeSecurityResult == CO_E_NOTINITIALIZED)
    {
        if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        {
            initializeSecurityResult = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
        }
    }
    if ( (SUCCEEDED(initializeSecurityResult) || initializeSecurityResult == RPC_E_TOO_LATE /* already called */)
            && SUCCEEDED(CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService))
            && SUCCEEDED(pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t()))
            && SUCCEEDED(pService->GetFolder(_bstr_t( L"\\"), &pRootFolder)))
    {
        if (pRootFolder->CreateFolder(_bstr_t(L"MEGA"), _variant_t(L""), &pMEGAFolder) == 0x800700b7)
        {
            pRootFolder->GetFolder(_bstr_t(L"MEGA"), &pMEGAFolder);
        }

        if (pMEGAFolder)
        {
            pMEGAFolder->DeleteTask(taskName, 0);
            pMEGAFolder->Release();
        }
        pRootFolder->Release();
    }

    if (pService)
    {
        pService->Release();
    }
}

// Check if it's needed to start the local HTTP server
// for communications with the webclient
bool PlatformImplementation::shouldRunHttpServer()
{
    bool result = false;
    PROCESSENTRY32 entry = {0};
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if (Process32First(snapshot, &entry))
    {
        while (Process32Next(snapshot, &entry))
        {
            // The MEGA webclient sends request to MEGAsync to improve the
            // user experience. We check if web browsers are running because
            // otherwise it isn't needed to run the local web server for this purpose.
            // Here is the list or web browsers that allow HTTP communications
            // with 127.0.0.1 inside HTTPS webs.
            if (!_wcsicmp(entry.szExeFile, L"chrome.exe") // Chromium has the same process name on Windows
                    || !_wcsicmp(entry.szExeFile, L"firefox.exe"))
            {
                result = true;
                break;
            }
        }
    }
    CloseHandle(snapshot);
    return result;
}

bool PlatformImplementation::isUserActive()
{
    LASTINPUTINFO lii = {0};
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (!GetLastInputInfo(&lii))
    {
        return true;
    }

    if ((GetTickCount() - lii.dwTime) > Preferences::USER_INACTIVITY_MS)
    {
        return false;
    }
    return true;
}

QString PlatformImplementation::getDeviceName()
{
    // First, try to read maker and model
    QSettings settings (QLatin1String("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\BIOS"),
                        QSettings::NativeFormat);
    QString vendor (settings.value(QLatin1String("BaseBoardManufacturer"),
                                   QLatin1String("0")).toString());
    QString model (settings.value(QLatin1String("SystemProductName"),
                                  QLatin1String("0")).toString());

    QString deviceName = vendor + QLatin1String(" ") + model;
    // If failure, empty strings or defaultFactoryBiosName, give hostname.
    if ((vendor.isEmpty() && model.isEmpty()) || deviceName.contains(NotAllowedDefaultFactoryBiosName))
    {
        deviceName = QHostInfo::localHostName();
    }

    return deviceName;
}

void PlatformImplementation::calculateInfoDialogCoordinates(const QRect& rect, int* posx, int* posy)
{
    int xSign = 1;
    int ySign = 1;
    QPoint position;
    QRect screenGeometry;

    position = QCursor::pos();
    QScreen* currentScreen = QGuiApplication::screenAt(position);
    if (currentScreen)
    {
        screenGeometry = currentScreen->availableGeometry();

        QString otherInfo = QString::fromUtf8("pos = [%1,%2], name = %3").arg(position.x()).arg(position.y()).arg(currentScreen->name());
        logInfoDialogCoordinates("availableGeometry", screenGeometry, otherInfo);

        if (!screenGeometry.isValid())
        {
            screenGeometry = currentScreen->geometry();
            otherInfo = QString::fromUtf8("dialog rect = %1").arg(rectToString(rect));
            logInfoDialogCoordinates("screenGeometry", screenGeometry, otherInfo);

            if (screenGeometry.isValid())
            {
                screenGeometry.setTop(28);
            }
            else
            {
                screenGeometry = rect;
                screenGeometry.setBottom(screenGeometry.bottom() + 4);
                screenGeometry.setRight(screenGeometry.right() + 4);
            }

            logInfoDialogCoordinates("screenGeometry 2", screenGeometry, otherInfo);
        }
        else
        {
            if (screenGeometry.y() < 0)
            {
                ySign = -1;
            }

            if (screenGeometry.x() < 0)
            {
                xSign = -1;
            }
        }

        QRect totalGeometry = QGuiApplication::primaryScreen()->geometry();
        APPBARDATA pabd;
        pabd.cbSize = sizeof(APPBARDATA);
        pabd.hWnd = FindWindow(L"Shell_TrayWnd", NULL);
        //TODO: the following only takes into account the position of the tray for the main screen.
        //Alternatively we might want to do that according to where the taskbar is for the targetted screen.
        if (pabd.hWnd && SHAppBarMessage(ABM_GETTASKBARPOS, &pabd)
                && pabd.rc.right != pabd.rc.left && pabd.rc.bottom != pabd.rc.top)
        {
            int size;
            switch (pabd.uEdge)
            {
            case ABE_LEFT:
                position = screenGeometry.bottomLeft();
                if (totalGeometry == screenGeometry)
                {
                    size = pabd.rc.right - pabd.rc.left;
                    size = size * screenGeometry.height() / (pabd.rc.bottom - pabd.rc.top);
                    screenGeometry.setLeft(screenGeometry.left() + size);
                }
                break;
            case ABE_RIGHT:
                position = screenGeometry.bottomRight();
                if (totalGeometry == screenGeometry)
                {
                    size = pabd.rc.right - pabd.rc.left;
                    size = size * screenGeometry.height() / (pabd.rc.bottom - pabd.rc.top);
                    screenGeometry.setRight(screenGeometry.right() - size);
                }
                break;
            case ABE_TOP:
                position = screenGeometry.topRight();
                if (totalGeometry == screenGeometry)
                {
                    size = pabd.rc.bottom - pabd.rc.top;
                    size = size * screenGeometry.width() / (pabd.rc.right - pabd.rc.left);
                    screenGeometry.setTop(screenGeometry.top() + size);
                }
                break;
            case ABE_BOTTOM:
                position = screenGeometry.bottomRight();
                if (totalGeometry == screenGeometry)
                {
                    size = pabd.rc.bottom - pabd.rc.top;
                    size = size * screenGeometry.width() / (pabd.rc.right - pabd.rc.left);
                    screenGeometry.setBottom(screenGeometry.bottom() - size);
                }
                break;
            }

            MegaApi::log(MegaApi::LOG_LEVEL_DEBUG, QString::fromUtf8("Calculating Info Dialog coordinates. pabd.uEdge = %1, pabd.rc = %2")
                         .arg(pabd.uEdge)
                         .arg(QString::fromUtf8("[%1,%2,%3,%4]").arg(pabd.rc.left).arg(pabd.rc.top).arg(pabd.rc.right).arg(pabd.rc.bottom))
                         .toUtf8().constData());

        }

        if (position.x() * xSign > (screenGeometry.right() / 2) * xSign)
        {
            *posx = screenGeometry.right() - rect.width() - 2;
        }
        else
        {
            *posx = screenGeometry.left() + 2;
        }

        if (position.y() * ySign > (screenGeometry.bottom() / 2) * ySign)
        {
            *posy = screenGeometry.bottom() - rect.height() - 2;
        }
        else
        {
            *posy = screenGeometry.top() + 2;
        }
    }

    QString otherInfo = QString::fromUtf8("dialog rect = %1, posx = %2, posy = %3").arg(rectToString(rect)).arg(*posx).arg(*posy);
    logInfoDialogCoordinates("Final", screenGeometry, otherInfo);
}

QString PlatformImplementation::findMimeType(const QString &extensionWithDot)
{
    return findAssociatedData(ASSOCSTR_CONTENTTYPE, extensionWithDot);
}

QString PlatformImplementation::findAssociatedExecutable(const QString &extensionWithDot)
{
    return findAssociatedData(ASSOCSTR_EXECUTABLE, extensionWithDot);
}

QString PlatformImplementation::findAssociatedModernApp(const QString &extensionWithDot)
{
    const QString associatedAppRegKey = buildAssociatedRegKey(extensionWithDot);
    const QString appFileClass = readAppIdFromRegistry(associatedAppRegKey);
    if (!appFileClass.isEmpty())
    {
        const QString editCommandRegKey = buildEditCommandRegKey(appFileClass);
        return readEditCommand(editCommandRegKey);
    }
    return QString();
}

QString PlatformImplementation::getDefaultVideoPlayer()
{
    WCHAR buff[MAX_PATH];
    if (SHGetFolderPath(0, CSIDL_PROGRAM_FILESX86, NULL, SHGFP_TYPE_CURRENT, buff) == S_OK)
    {
        QString path = QString::fromUtf16((ushort *)buff);
        path.append(QString::fromUtf8("\\Windows Media Player\\wmplayer.exe"));
        if (QFile(path).exists())
        {
            return path;
        }
    }
    return QString();
}

bool PlatformImplementation::isTraditionalApp(const QString &app)
{
    return app.endsWith(QString::fromLatin1(".exe"));
}

QString PlatformImplementation::buildWindowsModernAppCommand(const QString &app)
{
    const QStringList appParts = app.split(QString::fromLatin1("_"));
    const QString appName = appParts.first();
    const QString appFamilyName = appName + QString::fromLatin1("_") + appParts.last();
    return QString::fromUtf8("shell:appsFolder\\%1!%2").arg(appFamilyName, appName);
}

QString PlatformImplementation::findAssociatedData(const ASSOCSTR type, const QString &extensionWithDot)
{
    QString data;

    DWORD length = 0;
    HRESULT ret = AssocQueryString(0, type, (LPCWSTR)extensionWithDot.utf16(),
                                   NULL, NULL, &length);
    if (ret == S_FALSE)
    {
        WCHAR *buffer = new WCHAR[length];
        ret = AssocQueryString(0, type, (LPCWSTR)extensionWithDot.utf16(),
                               NULL, buffer, &length);
        if (ret == S_OK)
        {
            data = QString::fromUtf16((ushort *)buffer);
        }
        delete [] buffer;
    }
    return data;
}

QString PlatformImplementation::buildAssociatedRegKey(const QString &extensionWithDot)
{
    const QString baseKey = QString::fromLatin1("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\UserChoice");
    return baseKey.arg(extensionWithDot);
}

QString PlatformImplementation::buildEditCommandRegKey(const QString &appId)
{
    const QString baseKey = QString::fromLatin1("SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\PackageRepository\\Extensions\\ProgIDs\\%1");
    return baseKey.arg(appId);
}

QString PlatformImplementation::readEditCommand(const QString &regKey)
{
    HKEY hSubKey;
    const bool ok = openRegistry(HKEY_LOCAL_MACHINE, regKey, hSubKey);
    if (!ok)
    {
        return QString();
    }

    const int valuesCount = getRegKeyValuesCount(hSubKey);
    if (valuesCount < 1)
    {
        return QString();
    }

    for (int i=0; i<valuesCount; ++i)
    {
        const QString keyValue = getRegKeyValue(hSubKey, i);
        if (!keyValue.isEmpty())
        {
            return keyValue;
        }
    }

    return QString();
}

QString PlatformImplementation::readAppIdFromRegistry(const QString &regKey)
{
    HKEY hSubKey;
    const bool ok = openRegistry(HKEY_CURRENT_USER, regKey, hSubKey);
    if (!ok)
    {
        return QString();
    }

    DWORD type;
    TCHAR value[MAX_PATH];
    DWORD valuelen = sizeof(value);
    LONG result = RegQueryValueEx(hSubKey, L"ProgId", NULL, &type, (LPBYTE)value, &valuelen);
    RegCloseKey(hSubKey);

    if (result != ERROR_SUCCESS || type != REG_SZ)
    {
        return QString();
    }

    return QString::fromWCharArray(value);
}

int PlatformImplementation::getRegKeyValuesCount(HKEY regKey)
{
    DWORD valuesCount = 0;
    LONG result = RegQueryInfoKey(regKey, NULL, NULL, NULL, NULL, NULL, NULL,
                             &valuesCount, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS)
    {
        return -1;
    }
    return static_cast<int>(valuesCount);
}

QString PlatformImplementation::getRegKeyValue(HKEY regKey, const int valueIndex)
{
    const int bufferSize = 16383;
    TCHAR  valueBuffer[bufferSize];
    DWORD outBufferSize = bufferSize;
    valueBuffer[0] = '\0';
    LONG result = RegEnumValue(regKey, valueIndex, valueBuffer, &outBufferSize,
                               NULL, NULL, NULL, NULL);
    return (result == ERROR_SUCCESS ) ? QString::fromWCharArray(valueBuffer) : QString();
}

bool PlatformImplementation::openRegistry(HKEY baseKey, const QString &regKeyPath, HKEY &openedKey)
{
    const std::wstring wideRegKey = regKeyPath.toStdWString();
    LONG result = RegOpenKeyEx(baseKey, wideRegKey.c_str(), 0, KEY_READ, &openedKey);
    return (result == ERROR_SUCCESS);
}

DriveSpaceData PlatformImplementation::getDriveData(const QString &path)
{
    DriveSpaceData data;
    // Network paths in Windows such as \\<ip>\<dir> are not handled by QStorageInfo,
    // so we can't get available space this way.
    const QByteArray pathArray = path.toUtf8();

    ULARGE_INTEGER freeBytesAvailableToUser;
    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER totalFreeBytes;
    BOOL ok = GetDiskFreeSpaceExA(pathArray.constData(),
                                &freeBytesAvailableToUser,
                                &totalBytes,
                                &totalFreeBytes);
    data.mIsReady = (ok == TRUE);
    data.mAvailableSpace = totalFreeBytes.QuadPart;
    data.mTotalSpace = totalBytes.QuadPart;
    return data;
}
