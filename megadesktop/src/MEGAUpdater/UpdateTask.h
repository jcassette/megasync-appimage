#ifndef UPDATETASK_H
#define UPDATETASK_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cryptopp/cryptlib.h>
#include <cryptopp/modes.h>
#include <cryptopp/ccm.h>
#include <cryptopp/gcm.h>
#include <cryptopp/integer.h>
#include <cryptopp/aes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>
#include <cryptopp/rsa.h>
#include <cryptopp/crc.h>
#include <cryptopp/nbtheory.h>
#include <cryptopp/algparam.h>
#include <cryptopp/hmac.h>
#include <cryptopp/pwdbased.h>

namespace
{
#if CRYPTOPP_VERSION >= 600 && ((__cplusplus >= 201103L) || (__RPCNDR_H_VERSION__ == 500))
using byte = CryptoPP::byte;
#elif __RPCNDR_H_VERSION__ != 500
typedef unsigned char byte;
#endif

class Base64
{
    static byte to64(byte);
    static byte from64(byte);

public:
    static int btoa(const std::string&, std::string&);
    static int btoa(const byte*, int, char*);
    static int atob(const std::string&, std::string&);
    static int atob(const char*, byte*, int);
};

class SignatureChecker
{
public:
    SignatureChecker(const char *base64Key);
    ~SignatureChecker();

    void init();
    void add(const char *data, size_t size);
    bool checkSignature(const char *base64Signature);

protected:
    CryptoPP::Integer key[2];
    CryptoPP::SHA512 hash;
};
} // end of namespace

class UpdateTask
{
public:
    explicit UpdateTask();
    ~UpdateTask();
    void checkForUpdates();

protected:
    bool downloadFile(std::string url, std::string dstPath);
    bool processUpdateFile(FILE *fd);
    void processSymLinks(std::string symLinksPath);
    bool processSymLinksFile(FILE *fd);
    bool fileExist(const char* path);
    void initSignature();
    void addToSignature(const char *bytes, size_t length);
    bool checkSignature(std::string value);
    bool alreadyInstalled(std::string relativePath, std::string fileSignature);
    bool alreadyDownloaded(std::string relativePath, std::string fileSignature);
    bool alreadyExists(std::string absolutePath, std::string fileSignature);
    bool performUpdate();
    void rollbackUpdate(int fileNum);
    void initialCleanup();
    void finalCleanup();
    bool setPermissions(const char *path);
    bool removeRecursively(std::string path);
    int readVersion();
    void writeVersion();
    std::string getAppDataDir();
    std::string getAppDir();
    std::string readNextLine(FILE *fd);
    void emptydirlocal(std::string* name, dev_t basedev = 0);

    std::string appFolder;
    std::string appDataFolder;
    std::string updateFolder;
    std::string backupFolder;
    bool isPublic;
    SignatureChecker *signatureChecker;
    unsigned int currentFile;
    int updateVersion;
    std::vector<std::string> downloadURLs;
    std::vector<std::string> localPaths;
    std::vector<std::string> fileSignatures;
};

#endif // UPDATETASK_H
