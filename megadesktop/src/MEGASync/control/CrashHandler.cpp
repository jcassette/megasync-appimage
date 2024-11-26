#include "CrashHandler.h"
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QCoreApplication>
#include <QString>
#include <QDateTime>
#include <sstream>
#include "MegaApplication.h"

using namespace mega;
using namespace std;

#if defined(Q_OS_MAC)
#include "client/mac/handler/exception_handler.h"
#elif defined(Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined(Q_OS_WIN32)
#include "client/windows/handler/exception_handler.h"
#endif

#ifndef WIN32
    #ifndef CREATE_COMPATIBLE_MINIDUMPS

    #include <signal.h>
    #include <execinfo.h>
    #include <sys/utsname.h>


#ifdef __linux__
    #include <fstream>

string &ltrimEtcProperty(string &s, const char &c)
{
    size_t pos = s.find_first_not_of(c);
    s = s.substr(pos == string::npos ? s.length() : pos, s.length());
    return s;
}

string &rtrimEtcProperty(string &s, const char &c)
{
    size_t pos = s.find_last_not_of(c);
    if (pos != string::npos)
    {
        pos++;
    }
    s = s.substr(0, pos);
    return s;
}

string &trimEtcproperty(string &what)
{
    rtrimEtcProperty(what,' ');
    ltrimEtcProperty(what,' ');
    if (what.size() > 1)
    {
        if (what[0] == '\'' || what[0] == '"')
        {
            rtrimEtcProperty(what, what[0]);
            ltrimEtcProperty(what, what[0]);
        }
    }
    return what;
}

string getPropertyFromEtcFile(const char *configFile, const char *propertyName)
{
    ifstream infile(configFile);
    string line;

    while (getline(infile, line))
    {
        if (line.length() > 0 && line[0] != '#')
        {
            if (!strlen(propertyName)) //if empty return first line
            {
                return trimEtcproperty(line);
            }
            string key, value;
            size_t pos = line.find("=");
            if (pos != string::npos && ((pos + 1) < line.size()))
            {
                key = line.substr(0, pos);
                rtrimEtcProperty(key, ' ');

                if (!strcmp(key.c_str(), propertyName))
                {
                    value = line.substr(pos + 1);
                    return trimEtcproperty(value);
                }
            }
        }
    }

    return string();
}

string getDistro()
{
    string distro;
    distro = getPropertyFromEtcFile("/etc/lsb-release", "DISTRIB_ID");
    if (!distro.size())
    {
        distro = getPropertyFromEtcFile("/etc/os-release", "ID");
    }
    if (!distro.size())
    {
        distro = getPropertyFromEtcFile("/etc/redhat-release", "");
    }
    if (!distro.size())
    {
        distro = getPropertyFromEtcFile("/etc/debian-release", "");
    }
    if (distro.size() > 20)
    {
        distro = distro.substr(0, 20);
    }
    transform(distro.begin(), distro.end(), distro.begin(), ::tolower);
    return distro;
}

string getDistroVersion()
{
    string version;
    version = getPropertyFromEtcFile("/etc/lsb-release", "DISTRIB_RELEASE");
    if (!version.size())
    {
        version = getPropertyFromEtcFile("/etc/os-release", "VERSION_ID");
    }
    if (version.size() > 10)
    {
        version = version.substr(0, 10);
    }
    transform(version.begin(), version.end(), version.begin(), ::tolower);
    return version;
}
#endif


    string dump_path;

    // signal handler
    void signal_handler(int sig, siginfo_t *info, void *secret)
    {
        if (g_megaSyncLogger)
        {
            g_megaSyncLogger->flushAndClose();
        }

        int dump_file = open(dump_path.c_str(),  O_WRONLY | O_CREAT, 0400);
        if (dump_file<0)
        {
            CrashHandler::tryReboot();
            exit(128+sig);
        }

        std::ostringstream oss;
        oss << "MEGAprivate ERROR DUMP\n";
        oss << "Application: " << QApplication::applicationName().toUtf8().constData() << (sizeof(char*) == 4 ? " [32 bit]" : "") << (sizeof(char*) == 8 ? " [64 bit]" : "") << "\n";
        oss << "Version code: " << QString::number(Preferences::VERSION_CODE).toUtf8().constData() <<
               "." << QString::number(Preferences::BUILD_ID).toUtf8().constData() << "\n";
        oss << "Module name: " << MegaSyncApp->getMEGAString().toUtf8().constData() << "\n";
        oss << "Timestamp: " << QDateTime::currentMSecsSinceEpoch() << "\n";

        string distroinfo;
        #ifdef __linux__
            string distro = getDistro();
            if (distro.size())
            {
                distroinfo.append(distro);
                string distroversion = getDistroVersion();
                if (distroversion.size())
                {
                    distroinfo.append(" ");
                    distroinfo.append(distroversion);
                    distroinfo.append("/");
                }
                else
                {
                    distroinfo.append("/");
                }
            }
        #endif

        struct utsname osData;
        if (!uname(&osData))
        {
            oss << "Operating system: " << osData.sysname << "\n";
            oss << "System version:  " << distroinfo << osData.version << "\n";
            oss << "System release:  " << osData.release << "\n";
            oss << "System arch: " << osData.machine << "\n";
        }
        else
        {
            oss << "Operating system: Unknown\n";
            if (distroinfo.size())
            {
                oss << "System version: " << distroinfo << "\n";
            }
            else
            {
                oss << "System version: Unknown\n";
            }
            oss << "System release: Unknown\n";
            oss << "System arch: Unknown\n";
        }

        time_t rawtime;
        time(&rawtime);
        oss << "Error info:\n";
        if (info)
        {
            oss << strsignal(sig) << " (" << sig << ") at address " << std::showbase << std::hex << info->si_addr << std::dec << "\n";
        }
        else
        {
            oss << "Out of memory" << endl;
        }

        void *pnt = NULL;
        if (secret)
        {
            #if defined(__APPLE__)
                ucontext_t* uc = (ucontext_t*) secret;
                #if defined(__arm64__)
                    pnt = (void *)uc->uc_mcontext->__ss.__pc;
                #else
                    pnt = (void *)uc->uc_mcontext->__ss.__rip;
                #endif
            #elif defined(__x86_64__)
                ucontext_t* uc = (ucontext_t*) secret;
                pnt = (void*) uc->uc_mcontext.gregs[REG_RIP] ;
            #elif (defined (__ppc__)) || (defined (__powerpc__))
                ucontext_t* uc = (ucontext_t*) secret;
                pnt = (void*) uc->uc_mcontext.regs->nip ;
            #elif defined(__sparc__)
                struct sigcontext* sc = (struct sigcontext*) secret;
                #if __WORDSIZE == 64
                    pnt = (void*) scp->sigc_regs.tpc ;
                #else
                    pnt = (void*) scp->si_regs.pc ;
                #endif
            #elif defined(__i386__)
                ucontext_t* uc = (ucontext_t*) secret;
                pnt = (void*) uc->uc_mcontext.gregs[REG_EIP];
            #elif defined(__arm__)
                ucontext_t* uc = (ucontext_t*) secret;
                pnt = (void*) uc->uc_mcontext.arm_pc;
            #else
                pnt = NULL;
            #endif
        }

        oss << "Stacktrace:\n";
        void *stack[32];
        size_t size;
        size = backtrace(stack, 32);
        if (size > 1)
        {
            stack[1] = pnt;
            char **messages = backtrace_symbols(stack, size);
            for (unsigned int i = 1; i < size; i++)
            {
                oss << messages[i] << "\n";
            }
        }
        else
        {
            oss << "Error getting stacktrace\n";
        }

        write(dump_file, oss.str().c_str(), oss.str().size());
        close(dump_file);

        struct sigaction sa;
        sa.sa_handler = SIG_DFL;
        sigemptyset (&sa.sa_mask);
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGILL, &sa, NULL);
        sigaction(SIGFPE, &sa, NULL);
        sigaction(SIGABRT, &sa, NULL);
#if defined(__arm64__)
        sigaction(SIGTRAP, &sa, NULL);
#endif

        CrashHandler::tryReboot();
        exit(128+sig);
    }

    void mega_new_handler()
    {
        signal_handler(0, 0, 0);
    }

    #endif
#endif

/************************************************************************/
/* CrashHandlerPrivate                                                  */
/************************************************************************/
class CrashHandlerPrivate
{
public:
    CrashHandlerPrivate()
    {
        pHandler = NULL;
    }

    ~CrashHandlerPrivate()
    {
        delete pHandler;
    }

    void InitCrashHandler(const QString& dumpPath);
    static google_breakpad::ExceptionHandler* pHandler;
    static bool bReportCrashesToSystem;
};

google_breakpad::ExceptionHandler* CrashHandlerPrivate::pHandler = NULL;
bool CrashHandlerPrivate::bReportCrashesToSystem = true;

/************************************************************************/
/* DumpCallback                                                         */
/************************************************************************/
#if defined(Q_OS_WIN32)
bool DumpCallback(const wchar_t* _dump_dir,const wchar_t* _minidump_id,void* context,EXCEPTION_POINTERS* exinfo,MDRawAssertionInfo* assertion,bool success)
#elif defined(Q_OS_LINUX)
bool DumpCallback(const google_breakpad::MinidumpDescriptor &,void *context, bool success)
#elif defined(Q_OS_MAC)
bool DumpCallback(const char* _dump_dir,const char* _minidump_id,void *context, bool success)
#endif
{
    Q_UNUSED(context);
#if defined(Q_OS_WIN32)
    Q_UNUSED(_dump_dir);
    Q_UNUSED(_minidump_id);
    Q_UNUSED(assertion);
    Q_UNUSED(exinfo);
#endif

    if (g_megaSyncLogger)
    {
        g_megaSyncLogger->flushAndClose();
    }
    CrashHandler::tryReboot();
    return CrashHandlerPrivate::bReportCrashesToSystem ? success : false;
}

void CrashHandlerPrivate::InitCrashHandler(const QString& dumpPath)
{
    if ( pHandler != NULL )
        return;

#if defined(Q_OS_WIN32)
    std::wstring pathAsStr = (const wchar_t*)dumpPath.utf16();
    pHandler = new google_breakpad::ExceptionHandler(
        pathAsStr,
        /*FilterCallback*/ NULL,
        DumpCallback,
        /*context*/
        NULL,
        google_breakpad::ExceptionHandler::HANDLER_EXCEPTION
        );
#else
    #ifdef CREATE_COMPATIBLE_MINIDUMPS
        #if defined(Q_OS_LINUX)
            std::string pathAsStr = dumpPath.toUtf8().constData();
            google_breakpad::MinidumpDescriptor md(pathAsStr);
            pHandler = new google_breakpad::ExceptionHandler(
                md,
                /*FilterCallback*/ 0,
                DumpCallback,
                /*context*/ 0,
                true,
                -1
                );
        #elif defined(Q_OS_MAC)
            std::string pathAsStr = dumpPath.toUtf8().constData();
            pHandler = new google_breakpad::ExceptionHandler(
                pathAsStr,
                /*FilterCallback*/ 0,
                DumpCallback,
                /*context*/
                0,
                true,
                NULL
                );
        #endif
    #else
        srandom(time(NULL));
        uint32_t data1 = (uint32_t)random();
        uint16_t data2 = (uint16_t)random();
        uint16_t data3 = (uint16_t)random();
        uint32_t data4 = (uint32_t)random();
        uint32_t data5 = (uint32_t)random();

        char name[37];
        sprintf(name, "%08x-%04x-%04x-%08x-%08x", data1, data2, data3, data4, data5);
        dump_path = std::string(dumpPath.toUtf8().constData()) + "/" + name + ".dmp";

        /* Install our signal handler */
        struct sigaction sa;
        sa.sa_sigaction = signal_handler;
        sigemptyset (&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGILL, &sa, NULL);
        sigaction(SIGFPE, &sa, NULL);
        sigaction(SIGABRT, &sa, NULL);
// sigaction SIGTRAP for arm64 arch on macOS because in some cases such signal is emmited to indicate
// unhandled exceptions in the program (e.g seg fault)
#if defined(__APPLE__) && defined(__arm64__)
        sigaction(SIGTRAP, &sa, NULL);
#endif
        std::set_new_handler(mega_new_handler);
    #endif
#endif
}

/************************************************************************/
/* CrashHandler                                                         */
/************************************************************************/
CrashHandler* CrashHandler::instance()
{
    static CrashHandler globalHandler;
    return &globalHandler;
}

void CrashHandler::tryReboot()
{
    auto preferences = Preferences::instance();
    MegaApi::log(MegaApi::LOG_LEVEL_DEBUG, "Setting isCrashed true: tryReboot (CrashHandler)");
    preferences->setCrashed(true);

    if ((QDateTime::currentMSecsSinceEpoch()-preferences->getLastReboot()) > Preferences::MIN_REBOOT_INTERVAL_MS)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_WARNING, "Restarting app...");
        preferences->setLastReboot(QDateTime::currentMSecsSinceEpoch());

#ifndef __APPLE__
        QString app = MegaApplication::applicationFilePath();
        QProcess::startDetached(app, {});
#else
        QString app = MegaApplication::applicationDirPath();
        QString launchCommand = QString::fromUtf8("open");
        QStringList args = QStringList();

        QDir appPath(app);
        appPath.cdUp();
        appPath.cdUp();

        args.append(QString::fromLatin1("-n"));
        args.append(appPath.absolutePath());
        QProcess::startDetached(launchCommand, args);
#endif

#ifdef WIN32
        Sleep(2000);
#else
        sleep(2);
#endif
    }
    else
    {
        MegaApi::log(MegaApi::LOG_LEVEL_WARNING, "The app was recently restarted. Restart skipped");
    }
}

CrashHandler::CrashHandler() : QObject()
{
    d = new CrashHandlerPrivate();
    crashPostTimer.setSingleShot(true);
    connect(&crashPostTimer, SIGNAL(timeout()), this, SLOT(onCrashPostTimeout()));
    networkManager = NULL;
}

CrashHandler::~CrashHandler()
{
    delete d;
}

QString CrashHandler::getLastCrashHash() const
{
    return lastCrashHash;
}

void CrashHandler::setReportCrashesToSystem(bool report)
{
    d->bReportCrashesToSystem = report;
}

bool CrashHandler::writeMinidump()
{
    bool res = d->pHandler->WriteMinidump();
    if (res) {
        qDebug("BreakpadQt: writeMinidump() successed.");
    } else {
        qWarning("BreakpadQt: writeMinidump() failed.");
    }
    return res;
}

QStringList CrashHandler::getPendingCrashReports()
{
    auto preferences = Preferences::instance();
    QStringList previousCrashes = preferences->getPreviousCrashes();
    QStringList result;

    lastCrashHash.clear();

    MegaApi::log(MegaApi::LOG_LEVEL_INFO, "Checking pending crash repors");
    QDir dir(dumpPath);
    QFileInfoList fiList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    for (int i = 0; i < fiList.size(); i++)
    {
        QFile file(fiList[i].absoluteFilePath());
        if (!file.fileName().endsWith(QString::fromLatin1(".dmp")))
        {
            continue;
        }

        if (file.size() > 16384)
        {
            continue;
        }

        if (!file.open(QIODevice::ReadOnly))
        {
            continue;
        }

        QString crashReport = QString::fromUtf8(file.readAll());
        file.close();

        QStringList lines = crashReport.split(QString::fromLatin1("\n"));
        if ((lines.size()<3)
                || (lines.at(0) != QString::fromLatin1("MEGAprivate ERROR DUMP"))
                || (!lines.at(1).startsWith(QString::fromLatin1("Application: ") + QApplication::applicationName()))
                || (!lines.at(2).startsWith(QString::fromLatin1("Version code: ") + QString::number(Preferences::VERSION_CODE))))
        {
            MegaApi::log(MegaApi::LOG_LEVEL_WARNING, QString::fromUtf8("Invalid or outdated dump file: %1").arg(file.fileName()).toUtf8().constData());
            file.remove();
            continue;
        }

        QString crashHash = QString::fromLatin1(QCryptographicHash::hash(crashReport.toUtf8(),QCryptographicHash::Md5).toHex());
        if (lastCrashHash.isNull())
        {
            lastCrashHash = crashHash;
        }

        if (!previousCrashes.contains(crashHash))
        {
            MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("New crash file: %1  Hash: %2")
                         .arg(file.fileName()).arg(crashHash).toUtf8().constData());
            int idx = crashReport.indexOf(QString::fromLatin1("Version code: "));
            crashReport.insert(idx, QString::fromUtf8("Hash: %1\n").arg(crashHash));
            result.append(crashReport);
            previousCrashes.append(crashHash);
        }
        else
        {
            MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Already sent log file: %1").arg(file.fileName()).toUtf8().constData());
            file.remove();
        }
    }
    return result;
}

void CrashHandler::sendPendingCrashReports(QString userMessage)
{
    if (networkManager)
    {
        return;
    }

    QStringList crashes = getPendingCrashReports();
    if (!crashes.size())
    {
        return;
    }

    crashes.append(userMessage);
    QString postString = crashes.join(QString::fromLatin1("------------------------------\n"));
    postString.append(QString::fromLatin1("\n------------------------------\n"));

    networkManager = new QNetworkAccessManager();
    connect(networkManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(onPostFinished(QNetworkReply*)), Qt::UniqueConnection);
    request.setUrl(Preferences::CRASH_REPORT_URL);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromLatin1("application/x-www-form-urlencoded"));

    MegaApi::log(MegaApi::LOG_LEVEL_INFO, "Sending crash reports");
    networkManager->post(request, postString.toUtf8());
    loop.exec();
}

void CrashHandler::discardPendingCrashReports()
{
    auto preferences = Preferences::instance();
    QStringList crashes = getPendingCrashReports();
    QStringList previousCrashes = preferences->getPreviousCrashes();
    for (int i = 0; i < crashes.size(); i++)
    {
        QString crashHash = QString::fromLatin1(QCryptographicHash::hash(crashes[i].toUtf8(),QCryptographicHash::Md5).toHex());
        if (!previousCrashes.contains(crashHash))
        {
            previousCrashes.append(crashHash);
        }
    }
    preferences->setPreviousCrashes(previousCrashes);
    deletePendingCrashReports();
}

void CrashHandler::onPostFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    crashPostTimer.stop();
    if (networkManager)
    {
        networkManager->deleteLater();
        networkManager = NULL;
    }
    loop.exit();
    QVariant statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    if (!statusCode.isValid() || (statusCode.toInt() != 200) || (reply->error() != QNetworkReply::NoError))
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error sending crash reports");
        return;
    }

    MegaApi::log(MegaApi::LOG_LEVEL_INFO, "Crash reports sent");
    discardPendingCrashReports();
}

void CrashHandler::onCrashPostTimeout()
{
    loop.exit();
    if (networkManager)
    {
        networkManager->deleteLater();
        networkManager = NULL;
    }

    MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Timeout sending crash reports");
    return;
}

void CrashHandler::deletePendingCrashReports()
{
    QDir dir(dumpPath);
    QFileInfoList fiList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    for (int i = 0; i < fiList.size(); i++)
    {
        QFileInfo fi = fiList[i];
        if (fi.fileName().endsWith(QString::fromLatin1(".dmp")))
        {
            QFile::remove(fi.absoluteFilePath());
        }
    }
}

void CrashHandler::Init( const QString& reportPath )
{
    this->dumpPath = reportPath;
    d->InitCrashHandler(reportPath);
}

void CrashHandler::Disable()
{
    delete d;
    d = NULL;
}
