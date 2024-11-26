#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctime>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
#include <Windows.h>
#include <Shlwapi.h>
#include <AccCtrl.h>
#include <Aclapi.h>
#include <urlmon.h>
#include <direct.h>
#include <io.h>
#include <algorithm>
#include <Shlobj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#include <cstdlib>

#include "UpdateTask.h"
#include "Preferences.h"
#include "MacUtils.h"

using std::string;
using CryptoPP::Integer;

#ifdef _WIN32

#ifndef PATH_MAX
    #define PATH_MAX _MAX_PATH
#endif

#define MEGA_SEPARATOR '\\'

void utf8ToUtf16(const char* utf8data, string* utf16string)
{
    if(!utf8data)
    {
        utf16string->clear();
        utf16string->append("", 1);
        return;
    }

    size_t size = strlen(utf8data) + 1;

    // make space for the worst case
    utf16string->resize(size * sizeof(wchar_t));

    // resize to actual result
    utf16string->resize(sizeof(wchar_t) * MultiByteToWideChar(CP_UTF8, 0, utf8data, int(size), (wchar_t*)utf16string->data(),
                                                              int(utf16string->size()) / sizeof(wchar_t) + 1));
    if (utf16string->size())
    {
        utf16string->resize(utf16string->size() - 1);
    }
    else
    {
        utf16string->append("", 1);
    }
}

void utf16ToUtf8(const wchar_t* utf16data, int utf16size, string* utf8string)
{
    if(!utf16size)
    {
        utf8string->clear();
        return;
    }

    utf8string->resize((utf16size + 1) * 4);

    utf8string->resize(WideCharToMultiByte(CP_UTF8, 0, utf16data,
        utf16size,
        (char*)utf8string->data(),
        int(utf8string->size() + 1),
        NULL, NULL));
}

int mega_mkdir(const char *path)
{
    string wpath;
    utf8ToUtf16(path, &wpath);
    wpath.append("", 1);
    return _wmkdir((LPCWSTR)wpath.data());
}

int mega_access(const char *path)
{
    string wpath;
    utf8ToUtf16(path, &wpath);
    wpath.append("", 1);
    return _waccess((LPCWSTR)wpath.data(), 0);
}

FILE *mega_fopen(const char *path, const char *mode)
{
    string wpath;
    utf8ToUtf16(path, &wpath);
    wpath.append("", 1);

    string wmode;
    utf8ToUtf16(mode, &wmode);
    wmode.append("", 1);
    return _wfopen((LPCWSTR)wpath.data(), (LPCWSTR)wmode.data());
}

int mega_remove(const char *path)
{
    string wpath;
    utf8ToUtf16(path, &wpath);
    wpath.append("", 1);
    return _wremove((LPCWSTR)wpath.data());
}

int mega_rename(const char *srcpath, const char *dstpath)
{
    string wsrcpath;
    utf8ToUtf16(srcpath, &wsrcpath);
    wsrcpath.append("", 1);

    string wdstpath;
    utf8ToUtf16(dstpath, &wdstpath);
    wdstpath.append("", 1);

    return _wrename((LPCWSTR)wsrcpath.data(),(LPCWSTR)wdstpath.data());
}

int mega_rmdir(const char *path)
{
    string wpath;
    utf8ToUtf16(path, &wpath);
    wpath.append("", 1);
    return _wrmdir((LPCWSTR)wpath.data());
}

string UpdateTask::getAppDataDir()
{
    string path;
    string wpath;
    wpath.resize(MAX_PATH * sizeof (wchar_t));
    if (SHGetSpecialFolderPathW(NULL, (LPWSTR)wpath.data(), CSIDL_LOCAL_APPDATA, FALSE))
    {
        utf16ToUtf8((LPWSTR)wpath.data(), lstrlen((LPCWSTR)wpath.data()), &path);
        path.append("\\Mega Limited\\MEGAsync\\");
    }
    return path;
}

string UpdateTask::getAppDir()
{
    string path;
    string wpath;
    wpath.resize(MAX_PATH * sizeof (wchar_t));
    if (SUCCEEDED(GetModuleFileNameW(NULL, (LPWSTR)wpath.data() , MAX_PATH))
            && SUCCEEDED(PathRemoveFileSpecW((LPWSTR)wpath.data())))
    {
        utf16ToUtf8((LPWSTR)wpath.data(), lstrlen((LPCWSTR)wpath.data()), &path);
        path.append("\\");
    }
    return path;
}

#define MEGA_TO_NATIVE_SEPARATORS(x) std::replace(x.begin(), x.end(), '/', '\\');
#define MEGA_SET_PERMISSIONS

#else

#define MEGA_SEPARATOR '/'
#define mega_mkdir(x) mkdir(x, S_IRWXU)
#define mega_access(x) access(x, F_OK)
#define mega_fopen fopen
#define mega_remove remove
#define mega_rename rename
#define mega_rmdir rmdir

string UpdateTask::getAppDataDir()
{
    string path;
    const char* home = getenv("HOME");
    if (home)
    {
        path.append(home);
        path.append("/Library/Application\\ Support/Mega\\ Limited/MEGAsync/");
    }
    return path;
}

#define MEGA_TO_NATIVE_SEPARATORS(x) std::replace(x.begin(), x.end(), '\\', '/');
#define MEGA_SET_PERMISSIONS chmod("/Applications/MEGAsync.app/Contents/MacOS/MEGAsync", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); \
                             chmod("/Applications/MEGAsync.app/Contents/MacOS/MEGAupdater", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); \
                             chmod("/Applications/MEGAsync.app/Contents/PlugIns/MEGAShellExtFinder.appex/Contents/MacOS/MEGAShellExtFinder", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);


string UpdateTask::getAppDir()
{
    return APP_DIR_BUNDLE;
}

#endif

#define mega_base_path(x) x.substr(0, x.find_last_of("/\\") + 1)

enum {
    LOG_LEVEL_FATAL = 0,   // Very severe error event that will presumably lead the application to abort.
    LOG_LEVEL_ERROR,   // Error information but will continue application to keep running.
    LOG_LEVEL_WARNING, // Information representing errors in application but application will keep running
    LOG_LEVEL_INFO,    // Mainly useful to represent current progress of application.
    LOG_LEVEL_DEBUG,   // Informational logs, that are useful for developers. Only applicable if DEBUG is defined.
    LOG_LEVEL_MAX
};

#define MAX_LOG_SIZE 1024
char log_message[MAX_LOG_SIZE];
#define LOG(logLevel, ...) snprintf(log_message, MAX_LOG_SIZE, __VA_ARGS__); \
                                   cout << log_message << endl;

int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p;
    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path) - 1)
    {
        return -1;
    }
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++)
    {
        if (*p == '\\' || *p == '/')
        {
            /* Temporarily truncate */
            *p = '\0';
            if (mega_mkdir(_path) != 0 && errno != EEXIST)
            {
                return -1;
            }
            *p = MEGA_SEPARATOR;
        }
    }

    if (mega_mkdir(_path) != 0 && errno != EEXIST)
    {
        return -1;
    }
    return 0;
}

UpdateTask::UpdateTask()
{
    isPublic = false;
    string updatePublicKey = UPDATE_PUBLIC_KEY;
    if (getenv("MEGA_UPDATE_PUBLIC_KEY"))
    {
        updatePublicKey = getenv("MEGA_UPDATE_PUBLIC_KEY");
    }

    signatureChecker = new SignatureChecker(updatePublicKey.c_str());
    currentFile = 0;
    appDataFolder = getAppDataDir();
    appFolder = getAppDir();
    updateFolder = appDataFolder + UPDATE_FOLDER_NAME + MEGA_SEPARATOR;
    backupFolder = appDataFolder + BACKUP_FOLDER_NAME + MEGA_SEPARATOR;

#ifdef _WIN32
    WCHAR commonPath[MAX_PATH + 1];
    if (SHGetSpecialFolderPathW(NULL, commonPath, CSIDL_COMMON_APPDATA, FALSE))
    {
        string wappFolder;
        int len = lstrlen(commonPath);
        utf8ToUtf16(appFolder.c_str(), &wappFolder);
        if (!memcmp(commonPath, (WCHAR *)wappFolder.data(), len * sizeof(WCHAR))
                && wappFolder.size() > (len * sizeof(WCHAR))
                && *((WCHAR *)(wappFolder.data() + (len * sizeof(WCHAR)))) == L'\\')
        {
            isPublic = true;
            LOG(LOG_LEVEL_INFO, "The installation is public");
        }
    }
#endif
}

UpdateTask::~UpdateTask()
{
    delete signatureChecker;
}

void UpdateTask::checkForUpdates()
{
    LOG(LOG_LEVEL_INFO, "Starting update check");

    // Create random sequence for http request
    string randomSec("?");
    for (int i = 0; i < 10; i++)
    {
        randomSec += char('A'+ (rand() % 26));
    }

    if (!appFolder.size() || !appDataFolder.size())
    {
        LOG(LOG_LEVEL_ERROR, "No app or data folder set");
        return;
    }

    string appData = appDataFolder;
    string updateFile = appData.append(UPDATE_FILENAME);

    string updateURL = UPDATE_CHECK_URL;
    if (getenv("MEGA_UPDATE_CHECK_URL"))
    {
        updateURL = getenv("MEGA_UPDATE_CHECK_URL");
    }
    if (downloadFile((char *)((updateURL + randomSec).c_str()), updateFile.c_str()))
    {
        FILE * pFile;
        pFile = mega_fopen(updateFile.c_str(), "r");
        if (!pFile)
        {
            LOG(LOG_LEVEL_ERROR, "Error opening update file");
            mega_remove(updateFile.c_str());
            return;
        }

        if (!processUpdateFile(pFile))
        {
            fclose(pFile);
            mega_remove(updateFile.c_str());
            return;
        }
        initialCleanup();
        fclose(pFile);
        mega_remove(updateFile.c_str());

        currentFile = 0;
        while (currentFile < downloadURLs.size())
        {
            if (!alreadyDownloaded(localPaths[currentFile], fileSignatures[currentFile]))
            {
                //Create the folder for the new file
                string localFile = updateFolder + localPaths[currentFile];
                if (mkdir_p(mega_base_path(localFile).c_str()) == -1)
                {
                    LOG(LOG_LEVEL_INFO, "Unable to create folder for file: %s", localFile.c_str());
                    return;
                }

                //Delete the file if exists
                if (fileExist(localFile.c_str()))
                {
                    mega_remove(localFile.c_str());
                }

                //Download file to specific folder
                if (downloadFile(string(downloadURLs[currentFile] + randomSec).c_str(), localFile))
                {
                    LOG(LOG_LEVEL_INFO, "File ready: %s", localPaths[currentFile].c_str());
                    if (!alreadyDownloaded(localPaths[currentFile], fileSignatures[currentFile]))
                    {
                        LOG(LOG_LEVEL_ERROR, "Signature of downloaded file doesn't match: %s",  localPaths[currentFile].c_str());
                        return;
                    }
                    LOG(LOG_LEVEL_INFO, "File signature OK: %s",  localPaths[currentFile].c_str());
                    currentFile++;
                    continue;
                }
                return;
            }

            LOG(LOG_LEVEL_INFO, "File already downloaded: %s",  localPaths[currentFile].c_str());
            currentFile++;
        }

        //All files have been processed. Apply update
        if (!performUpdate())
        {
            LOG(LOG_LEVEL_INFO, "Error applying update");
            return;
        }

        finalCleanup();
    }
    else
    {
        LOG(LOG_LEVEL_ERROR, "Unable to download file");
        return;
    }
}

bool UpdateTask::downloadFile(string url, string dstPath)
{
    LOG(LOG_LEVEL_INFO, "Downloading updated file from: %s",  url.c_str());

#ifdef _WIN32
    string wurl;
    string wdstPath;

    wurl.resize((url.size() + 1) * 4);
    utf8ToUtf16(url.c_str(), &wurl);
    wurl.append("", 1);

    wdstPath.resize((url.size() + 1) * 4);
    utf8ToUtf16(dstPath.c_str(), &wdstPath);
    wdstPath.append("", 1);

    HRESULT res = URLDownloadToFileW(NULL, (LPCWSTR)wurl.data(), (LPCWSTR)wdstPath.data(), 0, NULL);
    if (res != S_OK)
    {
       LOG(LOG_LEVEL_ERROR, "Unable to download file. Error code: %d", res);
       return false;
    }
#else
    bool success = downloadFileSynchronously(url, dstPath);
    if (!success)
    {
        LOG(LOG_LEVEL_ERROR, "Unable to download file.");
        return false;
    }
#endif

    LOG(LOG_LEVEL_INFO, "File downloaded OK");
    return true;
}

bool UpdateTask::processUpdateFile(FILE *fd)
{
    LOG(LOG_LEVEL_DEBUG, "Reading update info");
    string version = readNextLine(fd);
    if (version.empty())
    {
        LOG(LOG_LEVEL_WARNING, "Invalid update info");
        return false;
    }

    int currentVersion = readVersion();
    if (currentVersion == -1)
    {
        LOG(LOG_LEVEL_INFO,"Error reading file version (megasync.version)");
        return false;
    }

    updateVersion = atoi(version.c_str());
    if (updateVersion <= currentVersion)
    {
        LOG(LOG_LEVEL_INFO, "Update not needed. Last version: %d - Current version: %d", updateVersion, currentVersion);
        return false;
    }
    LOG(LOG_LEVEL_WARNING, "Update needed");

    string updateSignature = readNextLine(fd);
    if (updateSignature.empty())
    {
        LOG(LOG_LEVEL_ERROR,"Invalid update info (empty info signature)");
        return false;
    }

    initSignature();
    addToSignature(version.data(), version.length());

    while (true)
    {
        string url = readNextLine(fd);
        if (url.empty())
        {
            break;
        }

        string localPath = readNextLine(fd);
        if (localPath.empty())
        {
            LOG(LOG_LEVEL_ERROR,"Invalid update info (empty path)");
            return false;
        }

        string fileSignature = readNextLine(fd);
        if (fileSignature.empty())
        {
            LOG(LOG_LEVEL_ERROR,"Invalid update info (empty file signature)");
            return false;
        }

        addToSignature(url.data(), url.length());
        addToSignature(localPath.data(), localPath.length());
        addToSignature(fileSignature.data(), fileSignature.length());

        MEGA_TO_NATIVE_SEPARATORS(localPath);
        if (alreadyInstalled(localPath, fileSignature))
        {
            LOG(LOG_LEVEL_INFO, "File already installed: %s",  localPath.c_str());
            continue;
        }

        downloadURLs.push_back(url);
        localPaths.push_back(localPath);
        fileSignatures.push_back(fileSignature);
    }

    if (!downloadURLs.size())
    {
        LOG(LOG_LEVEL_WARNING, "All files are up to date");
        return false;
    }

    if (!checkSignature(updateSignature))
    {
        LOG(LOG_LEVEL_ERROR,"Invalid update info (invalid signature)");
        return false;
    }

    return true;
}

bool UpdateTask::fileExist(const char *path)
{
    return (mega_access(path) != -1);
}

void UpdateTask::addToSignature(const char* bytes, size_t length)
{
    signatureChecker->add(bytes, length);
}

void UpdateTask::initSignature()
{
    signatureChecker->init();
}

bool UpdateTask::checkSignature(string value)
{
    bool result = signatureChecker->checkSignature(value.c_str());
    if (!result)
    {
        LOG(LOG_LEVEL_ERROR, "Invalid signature");
    }

    return result;
}

bool UpdateTask::performUpdate()
{
    string symlinksPath;
    LOG(LOG_LEVEL_INFO, "Applying update...");
    for (vector<string>::size_type i = 0; i < localPaths.size(); i++)
    {
        string file = backupFolder + localPaths[i];
        if (mkdir_p(mega_base_path(file).c_str()) == -1)
        {
            LOG(LOG_LEVEL_ERROR, "Error creating backup folder for: %s",  file.c_str());
            rollbackUpdate(int(i));
            return false;
        }

        string origFile = appFolder + localPaths[i];
        if (mega_rename(origFile.c_str(), file.c_str()) && errno != ENOENT)
        {
            LOG(LOG_LEVEL_ERROR, "Error creating backup of file %s to %s",  origFile.c_str(), file.c_str());
            rollbackUpdate(int(i));
            return false;
        }

        if (mkdir_p(mega_base_path(origFile).c_str()) == -1)
        {
            LOG(LOG_LEVEL_ERROR, "Error creating target folder for: %s",  origFile.c_str());
            rollbackUpdate(int(i));
            return false;
        }
        setPermissions(mega_base_path(origFile).c_str());

        string update = updateFolder + localPaths[i];
        if (mega_rename(update.c_str(), origFile.c_str()))
        {
            LOG(LOG_LEVEL_ERROR, "Error installing file %s in %s",  update.c_str(), origFile.c_str());
            rollbackUpdate(int(i));
            return false;
        }
        setPermissions(origFile.c_str());


        auto pos = origFile.find_last_of("/");
        if (pos != string::npos)
        {
            if (origFile.substr(pos + 1) == "mega.links")
            {
                symlinksPath = origFile;
            }
        }

        LOG(LOG_LEVEL_INFO, "File correctly installed: %s",  localPaths[i].c_str());
    }

    if (!symlinksPath.empty())
        processSymLinks(symlinksPath);

    LOG(LOG_LEVEL_INFO, "Update successfully installed");
    return true;
}

void UpdateTask::rollbackUpdate(int fileNum)
{
    LOG(LOG_LEVEL_INFO, "Uninstalling update...");
    for (int i = fileNum; i >= 0; i--)
    {
        string origFile = appFolder + localPaths[i];
        mega_rename(origFile.c_str(), (updateFolder + localPaths[i]).c_str());
        mega_rename((backupFolder + localPaths[i]).c_str(), origFile.c_str());
        LOG(LOG_LEVEL_INFO, "File restored: %s",  localPaths[i].c_str());
    }
    LOG(LOG_LEVEL_INFO, "Update uninstalled");
}

void UpdateTask::initialCleanup()
{
    removeRecursively(backupFolder);
}

void UpdateTask::finalCleanup()
{
    removeRecursively(updateFolder);
    MEGA_SET_PERMISSIONS;
    writeVersion();
}

void UpdateTask::processSymLinks(string symlinksPath)
{
#ifndef _WIN32
    FILE * pFile;
    pFile = mega_fopen(symlinksPath.c_str(), "r");
    if (!pFile)
    {
        LOG(LOG_LEVEL_ERROR, "Error opening sym links file %s", symlinksPath.c_str());
        return;
    }

    if (!processSymLinksFile(pFile))
    {
        LOG(LOG_LEVEL_INFO, "Error applying sym links");
    }

    fclose(pFile);
}

bool UpdateTask::processSymLinksFile(FILE *fd)
{
    LOG(LOG_LEVEL_INFO, "Reading symlinks update info");

    bool success = true;
    string version = readNextLine(fd);
    if (version.empty())
    {
        LOG(LOG_LEVEL_WARNING, "Invalid update sym links info");
        return false;
    }

    LOG(LOG_LEVEL_INFO, "Processing symlinks");

    while (true)
    {
        string target = readNextLine(fd);
        string symLinkRelativePath = readNextLine(fd);
        if (target.empty() || symLinkRelativePath.empty())
        {
            break;
        }

        std::string linkPath = appFolder + symLinkRelativePath;
        LOG(LOG_LEVEL_INFO,"First origin link %s", target.c_str());
        LOG(LOG_LEVEL_INFO,"Second target link %s", linkPath.c_str());

        if (symlink(target.c_str(), linkPath.c_str()) != 0 && errno != EEXIST)
        {
            LOG(LOG_LEVEL_ERROR, "Error creating symlinks");
            std::cerr << "Failed to create symlink " << linkPath << " -> " << target << ": " << strerror(errno) << std::endl;
            success = false;
        }
    }

    return success;
#endif
}

bool UpdateTask::setPermissions(const char *path)
{
#ifndef _WIN32
    return true;
#else
    if (!isPublic)
    {
        return true;
    }

    string wfileName;
    utf8ToUtf16(path, &wfileName);
    wfileName.append("", 1);

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
    if ((GetNamedSecurityInfo((LPCWSTR)wfileName.data(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD) == ERROR_SUCCESS)
            && (ea.Trustee.ptstrName = (LPWSTR)LocalAlloc(LMEM_FIXED, sidSize))
            && CreateWellKnownSid(WinBuiltinUsersSid, NULL, ea.Trustee.ptstrName, &sidSize)
            && (SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL) == ERROR_SUCCESS)
            && (SetNamedSecurityInfo((LPWSTR)wfileName.data(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDACL, NULL) == ERROR_SUCCESS))
    {
        result = true;
        LOG(LOG_LEVEL_INFO, "Permissions updated");
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
#endif
}

bool UpdateTask::removeRecursively(string path)
{
#ifndef _WIN32
    string spath = path;
    emptydirlocal(&spath);
#else
    string utf16path;
    utf8ToUtf16(path.c_str(), &utf16path);
    if (utf16path.size() > 1)
    {
        utf16path.resize(utf16path.size() - 1);
        emptydirlocal(&utf16path);
    }
#endif

    return !mega_rmdir(path.c_str());
}

bool UpdateTask::alreadyInstalled(string relativePath, string fileSignature)
{
    return alreadyExists(appFolder + relativePath, fileSignature);
}

bool UpdateTask::alreadyDownloaded(string relativePath, string fileSignature)
{
    return alreadyExists(updateFolder + relativePath, fileSignature);
}

bool UpdateTask::alreadyExists(string absolutePath, string fileSignature)
{
    string updatePublicKey = UPDATE_PUBLIC_KEY;
    if (getenv("MEGA_UPDATE_PUBLIC_KEY"))
    {
        updatePublicKey = getenv("MEGA_UPDATE_PUBLIC_KEY");
    }
    SignatureChecker tmpHash(updatePublicKey.c_str());
    char *buffer;
    long fileLength;
    FILE * pFile = mega_fopen(absolutePath.c_str(), "rb");
    if (pFile == NULL)
    {
        return false;
    }

    //Get size of file and rewind FILE pointer
    fseek(pFile, 0, SEEK_END);
    fileLength = ftell(pFile);
    rewind(pFile);

    buffer = (char *)malloc(fileLength);
    if (buffer == NULL)
    {
        fclose(pFile);
        return false;
    }

    size_t sizeRead = fread(buffer, 1, fileLength, pFile);
    if (sizeRead != fileLength)
    {
        fclose(pFile);
        return false;
    }

    tmpHash.add(buffer, sizeRead);
    fclose(pFile);
    free(buffer);

    return tmpHash.checkSignature(fileSignature.data());
}

string UpdateTask::readNextLine(FILE *fd)
{
    char line[4096];
    if (!fgets(line, sizeof(line), fd))
    {
        return string();
    }

    line[strcspn(line, "\n")] = '\0';
    return string(line);
}

int UpdateTask::readVersion()
{
    int version = -1;
    FILE *fp = mega_fopen((appDataFolder + VERSION_FILE_NAME).c_str(), "r");
    if (fp == NULL)
    {
        return version;
    }

    fscanf(fp, "%d", &version);
    fclose(fp);
    return version;
}

void UpdateTask::writeVersion()
{
    FILE *fp = mega_fopen((appDataFolder + VERSION_FILE_NAME).c_str(), "w");
    if (fp == NULL)
    {
        return;
    }

    fprintf(fp, "%d", updateVersion);
    fclose(fp);
    LOG(LOG_LEVEL_INFO, "Version code updated to %d", updateVersion);
}

#ifdef _WIN32
// delete all files and folders contained in the specified folder
// (does not recurse into mounted devices)
void UpdateTask::emptydirlocal(string* name, dev_t basedev)
{
    HANDLE hDirectory, hFind;
    dev_t currentdev;

    int added = 0;
    if (name->size() > sizeof(wchar_t) && !memcmp(name->data() + name->size() - sizeof(wchar_t), (char*)(void*)L":", sizeof(wchar_t)))
    {
        name->append((char*)(void*)L"\\", sizeof(wchar_t));
        added = sizeof(wchar_t);
    }

    name->append("", 1);

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW((LPCWSTR)name->data(), GetFileExInfoStandard, (LPVOID)&fad)
        || !(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        || fad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        // discard files and resparse points (links, etc.)
        name->resize(name->size() - added - 1);
        return;
    }

    hDirectory = CreateFileW((LPCWSTR)name->data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    name->resize(name->size() - added - 1);
    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        // discard not accessible folders
        return;
    }

    BY_HANDLE_FILE_INFORMATION fi;
    if (!GetFileInformationByHandle(hDirectory, &fi))
    {
        currentdev = 0;
    }
    else
    {
        currentdev = fi.dwVolumeSerialNumber + 1;
    }
    CloseHandle(hDirectory);
    if (basedev && currentdev != basedev)
    {
        // discard folders on different devices
        return;
    }

    bool removed;
    for (;;)
    {
        // iterate over children and delete
        removed = false;
        name->append((char*)(void*)L"\\*", 5);
        WIN32_FIND_DATAW ffd;
        hFind = FindFirstFileW((LPCWSTR)name->data(), &ffd);
        name->resize(name->size() - 5);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            break;
        }

        bool morefiles = true;
        while (morefiles)
        {
            if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                && (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    || *ffd.cFileName != '.'
                    || (ffd.cFileName[1] && ((ffd.cFileName[1] != '.')
                    || ffd.cFileName[2]))))
            {
                string childname = *name;
                childname.append((char*)(void*)L"\\", 2);
                childname.append((char*)ffd.cFileName, sizeof(wchar_t) * wcslen(ffd.cFileName));
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    emptydirlocal(&childname , currentdev);
                    childname.append("", 1);
                    removed |= !!RemoveDirectoryW((LPCWSTR)childname.data());
                }
                else
                {
                    childname.append("", 1);
                    removed |= !!DeleteFileW((LPCWSTR)childname.data());
                }
            }
            morefiles = FindNextFileW(hFind, &ffd);
        }

        FindClose(hFind);
        if (!removed)
        {
            break;
        }
    }
}
#else
// delete all files, folders and symlinks contained in the specified folder
// (does not recurse into mounted devices)
void UpdateTask::emptydirlocal(string* name, dev_t basedev)
{
#ifdef USE_IOS
    string absolutename;
    if (appbasepath)
    {
        if (name->size() && name->at(0) != '/')
        {
            absolutename = appbasepath;
            absolutename.append(*name);
            name = &absolutename;
        }
    }
#endif

    DIR* dp;
    dirent* d;
    int removed;
    struct stat statbuf;
    int t;

    if (!basedev)
    {
        if (lstat(name->c_str(), &statbuf) || !S_ISDIR(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) return;
        basedev = statbuf.st_dev;
    }

    if ((dp = opendir(name->c_str())))
    {
        for (;;)
        {
            removed = 0;

            while ((d = readdir(dp)))
            {
                if (d->d_type != DT_DIR
                 || *d->d_name != '.'
                 || (d->d_name[1] && (d->d_name[1] != '.' || d->d_name[2])))
                {
                    t = name->size();
                    name->append("/");
                    name->append(d->d_name);

                    if (!lstat(name->c_str(), &statbuf))
                    {
                        if (!S_ISLNK(statbuf.st_mode) && S_ISDIR(statbuf.st_mode) && statbuf.st_dev == basedev)
                        {
                            emptydirlocal(name, basedev);
                            removed |= !rmdir(name->c_str());
                        }
                        else
                        {
                            removed |= !unlink(name->c_str());
                        }
                    }

                    name->resize(t);
                }
            }

            if (!removed)
            {
                break;
            }

            rewinddir(dp);
        }

        closedir(dp);
    }
}
#endif

namespace
{
SignatureChecker::SignatureChecker(const char *base64Key)
{
    string pubks;
    size_t len = strlen(base64Key)/4*3+3;
    pubks.resize(len);
    pubks.resize(Base64::atob(base64Key, (byte *)pubks.data(), int(len)));


    byte *data = (byte*)pubks.data();
    int datalen = int(pubks.size());

    int p, i, n;
    p = 0;

    for (i = 0; i < 2; i++)
    {
        if (p + 2 > datalen)
        {
            break;
        }

        n = ((data[p] << 8) + data[p + 1] + 7) >> 3;

        p += 2;
        if (p + n > datalen)
        {
            break;
        }

        key[i] = Integer(data + p, n);

        p += n;
    }

    assert(i == 2 && len - p < 16);
}

SignatureChecker::~SignatureChecker()
{

}

void SignatureChecker::init()
{
    string out;
    out.resize(hash.DigestSize());
    hash.Final((byte*)out.data());
}

void SignatureChecker::add(const char *data, size_t size)
{
    hash.Update((const byte *)data, size);
}

bool SignatureChecker::checkSignature(const char *base64Signature)
{
    byte signature[512];
    int l = Base64::atob(base64Signature, signature, sizeof(signature));
    if (l != sizeof(signature))
        return false;

    string h, s;
    unsigned size;

    h.resize(hash.DigestSize());
    hash.Final((byte*)h.data());

    s.resize(h.size());
    byte* buf = (byte *)s.data();


    Integer t (signature, sizeof(signature));
    t = a_exp_b_mod_c(t, key[1], key[0]);
    unsigned int i = t.ByteCount();
    if (i > s.size())
    {
        return 0;
    }

    while (i--)
    {
        *buf++ = t.GetByte(i);
    }

    size = t.ByteCount();
    if (!size)
    {
        return 0;
    }

    if (size < h.size())
    {
        // left-pad with 0
        s.insert(0, h.size() - size, 0);
        s.resize(h.size());
    }

    return s == h;
}

unsigned char Base64::to64(byte c)
{
    c &= 63;

    if (c < 26)
    {
        return c + 'A';
    }

    if (c < 52)
    {
        return c - 26 + 'a';
    }

    if (c < 62)
    {
        return c - 52 + '0';
    }

    if (c == 62)
    {
        return '-';
    }

    return '_';
}

unsigned char Base64::from64(byte c)
{
    if ((c >= 'A') && (c <= 'Z'))
    {
        return c - 'A';
    }

    if ((c >= 'a') && (c <= 'z'))
    {
        return c - 'a' + 26;
    }

    if ((c >= '0') && (c <= '9'))
    {
        return c - '0' + 52;
    }

    if (c == '-' || c == '+')
    {
        return 62;
    }

    if (c == '_' || c == '/')
    {
        return 63;
    }

    return 255;
}


int Base64::atob(const string &in, string &out)
{
    out.resize(in.size() * 3 / 4 + 3);
    out.resize(Base64::atob(in.data(), (byte *) out.data(), int(out.size())));

    return int(out.size());
}

int Base64::atob(const char* a, byte* b, int blen)
{
    byte c[4];
    int i;
    int p = 0;

    c[3] = 0;

    for (;;)
    {
        for (i = 0; i < 4; i++)
        {
            if ((c[i] = from64(*a++)) == 255)
            {
                break;
            }
        }

        if ((p >= blen) || !i)
        {
            return p;
        }

        b[p++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);

        if ((p >= blen) || (i < 3))
        {
            return p;
        }

        b[p++] = (c[1] << 4) | ((c[2] & 0x3c) >> 2);

        if ((p >= blen) || (i < 4))
        {
            return p;
        }

        b[p++] = (c[2] << 6) | c[3];
    }

    //return p; // warning C4702: unreachable code
}

int Base64::btoa(const string &in, string &out)
{
    out.resize(in.size() * 4 / 3 + 4);
    out.resize(Base64::btoa((const byte*) in.data(), int(in.size()), (char *) out.data()));

    return int(out.size());
}

int Base64::btoa(const byte* b, int blen, char* a)
{
    int p = 0;

    for (;;)
    {
        if (blen <= 0)
        {
            break;
        }

        a[p++] = to64(*b >> 2);
        a[p++] = to64((*b << 4) | (((blen > 1) ? b[1] : 0) >> 4));

        if (blen < 2)
        {
            break;
        }

        a[p++] = to64(b[1] << 2 | (((blen > 2) ? b[2] : 0) >> 6));

        if (blen < 3)
        {
            break;
        }

        a[p++] = to64(b[2]);

        blen -= 3;
        b += 3;
    }

    a[p] = 0;

    return p;
}
} // end of namespace
