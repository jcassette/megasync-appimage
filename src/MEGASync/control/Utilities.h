#ifndef UTILITIES_H
#define UTILITIES_H

#include "megaapi.h"
#include "ThreadPool.h"
#include <QString>
#include <QHash>
#include <QPixmap>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QFuture>
#include <QDir>
#include <QIcon>
#include <QLabel>
#include <QQueue>
#include <QEventLoop>
#include <QMetaEnum>
#include <QTimer>

#include <QEasingCurve>

#include <functional>

#include <sys/stat.h>

#ifdef __APPLE__
#define MEGA_SET_PERMISSIONS chmod("/Applications/MEGAsync.app/Contents/MacOS/MEGAsync", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); \
                             chmod("/Applications/MEGAsync.app/Contents/MacOS/MEGAupdater", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH); \
                             chmod("/Applications/MEGAsync.app/Contents/PlugIns/MEGAShellExtFinder.appex/Contents/MacOS/MEGAShellExtFinder", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif

#define MegaSyncApp (static_cast<MegaApplication *>(QCoreApplication::instance()))

template <typename E>
constexpr typename std::underlying_type<E>::type toInt(E e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

struct PlanInfo
{
    long long gbStorage;
    long long gbTransfer;
    unsigned int minUsers;
    int level;
    int gbPerStorage;
    int gbPerTransfer;
    unsigned int pricePerUserBilling;
    unsigned int pricePerUserLocal;
    unsigned int pricePerStorageBilling;
    unsigned int pricePerStorageLocal;
    unsigned int pricePerTransferBilling;
    unsigned int pricePerTransferLocal;
    QString billingCurrencySymbol;
    QString billingCurrencyName;
    QString localCurrencySymbol;
    QString localCurrencyName;
};

struct PSA_info
{
    int idPSA;
    QString title;
    QString desc;
    QString urlImage;
    QString textButton;
    QString urlClick;

    PSA_info()
    {
        clear();
    }

    PSA_info(const PSA_info& info)
    {
        idPSA = info.idPSA;
        title = info.title;
        desc = info.desc;
        urlImage = info.urlImage;
        textButton = info.textButton;
        urlClick = info.urlClick;
    }

    void clear()
    {
        idPSA = -1;
        title = QString();
        desc = QString();
        urlImage = QString();
        textButton = QString();
        urlClick = QString();
    }
};

namespace DeviceOs
{
Q_NAMESPACE

enum Os
{
    UNDEFINED,
    LINUX,
    MAC,
    WINDOWS
};
Q_ENUM_NS(Os)

inline DeviceOs::Os getCurrentOS()
{
#ifdef Q_OS_WINDOWS
    return DeviceOs::WINDOWS;
#endif
#ifdef Q_OS_MACOS
    return DeviceOs::MAC;
#endif
#ifdef Q_OS_LINUX
    return DeviceOs::LINUX;
#endif
}
}

class IStorageObserver
{
public:
    virtual ~IStorageObserver() = default;
    virtual void updateStorageElements() = 0;
};

class IBandwidthObserver
{
public:
    virtual ~IBandwidthObserver() = default;
    virtual void updateBandwidthElements() = 0;
};

class IAccountObserver
{
public:
    virtual ~IAccountObserver() = default;
    virtual void updateAccountElements() = 0;
};

class StorageDetailsObserved
{
public:
    virtual ~StorageDetailsObserved() = default;
    void attachStorageObserver(IStorageObserver& obs)
    {
        storageObservers.push_back(&obs);
    }
    void dettachStorageObserver(IStorageObserver& obs)
    {
        storageObservers.erase(std::remove(storageObservers.begin(), storageObservers.end(), &obs), storageObservers.end());
    }

    void notifyStorageObservers()
    {
        for (IStorageObserver* o : storageObservers)
        {
            o->updateStorageElements();
        }
    }

private:
    std::vector<IStorageObserver*> storageObservers;
};

class BandwidthDetailsObserved
{
public:
    virtual ~BandwidthDetailsObserved() = default;
    void attachBandwidthObserver(IBandwidthObserver& obs)
    {
        bandwidthObservers.push_back(&obs);
    }
    void dettachBandwidthObserver(IBandwidthObserver& obs)
    {
        bandwidthObservers.erase(std::remove(bandwidthObservers.begin(), bandwidthObservers.end(), &obs));
    }

    void notifyBandwidthObservers()
    {
        for (IBandwidthObserver* o : bandwidthObservers)
        {
            o->updateBandwidthElements();
        }
    }

private:
    std::vector<IBandwidthObserver*> bandwidthObservers;
};


class AccountDetailsObserved
{
public:
    virtual ~AccountDetailsObserved() = default;
    void attachAccountObserver(IAccountObserver& obs)
    {
        accountObservers.push_back(&obs);
    }
    void dettachAccountObserver(IAccountObserver& obs)
    {
        accountObservers.erase(std::remove(accountObservers.begin(), accountObservers.end(), &obs));
    }

    void notifyAccountObservers()
    {
        for (IAccountObserver* o : accountObservers)
        {
            o->updateAccountElements();
        }
    }

private:
    std::vector<IAccountObserver*> accountObservers;
};

class ThreadPoolSingleton
{
    private:
        static std::unique_ptr<ThreadPool> instance;
        ThreadPoolSingleton() {}

    public:
        static ThreadPool* getInstance()
        {
            if (instance == nullptr)
            {
                instance.reset(new ThreadPool(5));
            }

            return instance.get();
        }
};


/**
 * @brief The MegaListenerFuncExecuter class
 *
 * it takes an std::function as parameter that will be called upon request finish.
 *
 */
class MegaListenerFuncExecuter : public mega::MegaRequestListener
{
private:
    bool mAutoremove = true;
    bool mExecuteInAppThread = true;
    std::function<void(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError *e)> onRequestFinishCallback;

public:

    /**
     * @brief MegaListenerFuncExecuter
     * @param func to call upon onRequestFinish
     * @param autoremove whether this should be deleted after func is called
     */
    MegaListenerFuncExecuter(bool autoremove = false,
                             std::function<void(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError *e)> func = nullptr
                            )
        : mAutoremove(autoremove), onRequestFinishCallback(std::move(func))
    {
    }

    virtual void onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e);
    virtual void onRequestStart(mega::MegaApi*, mega::MegaRequest*) {}
    virtual void onRequestUpdate(mega::MegaApi*, mega::MegaRequest*) {}
    virtual void onRequestTemporaryError(mega::MegaApi*, mega::MegaRequest*, mega::MegaError*) {}

    void setExecuteInAppThread(bool executeInAppThread);
};

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags())
        : QLabel(parent)
    {
        Q_UNUSED(f)
#ifndef __APPLE__
        setMouseTracking(true);
#endif
    }

    ~ClickableLabel() {}

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent*)
    {
        emit clicked();
    }
#ifndef __APPLE__
    void enterEvent(QEvent*)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void leaveEvent(QEvent*)
    {
        setCursor(Qt::ArrowCursor);
    }
#endif
};

struct TimeInterval
{
    TimeInterval(long long secs, bool secondPrecision = true);

    TimeInterval& operator=(const TimeInterval& other);

    int days;
    int hours;
    int minutes;
    int seconds;
    bool useSecondPrecision;
};

class Utilities
{
public:
    enum class FileType
    {
        TYPE_OTHER    = 0x01,
        TYPE_AUDIO    = 0x02,
        TYPE_VIDEO    = 0x04,
        TYPE_ARCHIVE  = 0x08,
        TYPE_DOCUMENT = 0x10,
        TYPE_IMAGE    = 0x20,
    };
    Q_DECLARE_FLAGS(FileTypes, FileType)
    static const QString SUPPORT_URL;
    static const QString BACKUP_CENTER_URL;
    static const QString SYNC_SUPPORT_URL;

    static QString getSizeString(long long bytes);
    static QString getSizeStringLocalized(qint64 bytes);
    static int toNearestUnit(long long bytes);
    static QString getTranslatedSeparatorTemplate();

    struct ProgressSize
    {
        QString transferredBytes;
        QString totalBytes;
        QString units;
    };
    static ProgressSize getProgressSizes(long long transferredBytes, long long totalBytes);

    static QString createSimpleUsedString(long long usedData);
    static QString createSimpleUsedOfString(long long usedData, long long totalData);
    static QString createSimpleUsedStringWithoutReplacement(long long usedData);
    static QString createCompleteUsedString(long long usedData, long long totalData, int percentage);
    static QString getTimeString(long long secs, bool secondPrecision = true, bool color = true);
    static QString getAddedTimeString(long long secs);
    static QString extractJSONString(QString json, QString name);
    static QStringList extractJSONStringList(const QString& json, const QString& name);
    static long long extractJSONNumber(QString json, QString name);
    static QString getDefaultBasePath();
    static void getPROurlWithParameters(QString &url);
    static QString joinLogZipFiles(mega::MegaApi *megaApi, const QDateTime *timestampSince = nullptr, QString appendHashReference = QString());

    static void adjustToScreenFunc(QPoint position, QWidget *what);
    static QString minProPlanNeeded(std::shared_ptr<mega::MegaPricing> pricing, long long usedStorage);
    static QString getReadableStringFromTs(mega::MegaIntegerList* list);
    static QString getReadablePlanFromId(int identifier, bool shortPlan = false);
    static void animateFadeout(QWidget *object, int msecs = 700);
    static void animateFadein(QWidget *object, int msecs = 700);
    static void animatePartialFadeout(QWidget *object, int msecs = 2000);
    static void animatePartialFadein(QWidget *object, int msecs = 2000);
    static void animateProperty(QWidget *object, int msecs, const char *property, QVariant startValue, QVariant endValue, QEasingCurve curve = QEasingCurve::InOutQuad);
    // Returns remaining days until given Unix timestamp in seconds.
    static void getDaysToTimestamp(int64_t secsTimestamps, int64_t &remaininDays);
    // Returns remaining days / hours until given Unix timestamp in seconds.
    // Note: remainingHours and remaininDays represent the same value.
    // i.e. for 1 day & 3 hours remaining, remainingHours will be 27, not 3.
    static void getDaysAndHoursToTimestamp(int64_t secsTimestamps, int64_t &remaininDays, int64_t &remainingHours);

    static QString getNonDuplicatedNodeName(mega::MegaNode* node, mega::MegaNode* parentNode, const QString& currentName, bool unescapeName, const QStringList &itemsBeingRenamed);
    static QString getNonDuplicatedLocalName(const QFileInfo& currentFile, bool unescapeName, const QStringList &itemsBeingRenamed);
    static QPair<QString, QString> getFilenameBasenameAndSuffix(const QString& fileName);

    static void upgradeClicked();

    //get mega transfer nodepath
    static QString getNodePath(mega::MegaTransfer* transfer);

    //Check is current account is business (either business or flexi pro)
    static bool isBusinessAccount();
    static QFuture<bool> openUrl(QUrl url);
    static void openInMega(mega::MegaHandle handle);
    static void openBackupCenter();

    static QString getCommonPath(const QString& path1, const QString& path2, bool cloudPaths);

    static bool isIncommingShare(mega::MegaNode* node);

    static bool dayHasChangedSince(qint64 msecs);
    static bool monthHasChangedSince(qint64 msecs);

    static QString getTranslatedError(const mega::MegaError* error);

    static bool restoreNode(mega::MegaNode* node,
                            mega::MegaApi* megaApi,
                            bool async,
                            std::function<void(mega::MegaRequest*, mega::MegaError*)> finishFunc);

    static bool restoreNode(const char* nodeName,
                            const char* nodeDirectoryPath,
                            mega::MegaApi* megaApi,
                            std::function<void(mega::MegaRequest*, mega::MegaError*)> finishFunc);

private:
    Utilities() {}
    static QHash<QString, QString> extensionIcons;
    static QHash<QString, FileType> fileTypes;
    static QHash<QString, QString> languageNames;
    static void initializeExtensions();
    static void initializeFileTypes();
    static QString getExtensionPixmapNameSmall(QString fileName);
    static QString getExtensionPixmapNameMedium(QString fileName);
    static double toDoubleInUnit(unsigned long long bytes, unsigned long long unit);
    static QString getTimeFormat(const TimeInterval& interval);
    static QString filledTimeString(const QString& timeFormat, const TimeInterval& interval, bool color);

    static QString cleanedTimeString(const QString& timeString);

    static long long getNearestUnit(long long bytes);

//Platform dependent functions
public:
    static QString languageCodeToString(QString code);
    static QString getAvatarPath(QString email);
    static void removeAvatars();
    static bool removeRecursively(QString path);
    static void copyRecursively(QString srcPath, QString dstPath);

    static void queueFunctionInAppThread(std::function<void()> fun);
    static void queueFunctionInObjectThread(QObject* object, std::function<void()> fun);

    static void getFolderSize(QString folderPath, long long *size);
    static qreal getDevicePixelRatio();

    static QIcon getCachedPixmap(QString fileName);
    static QIcon getExtensionPixmapSmall(QString fileName);
    static QIcon getExtensionPixmapMedium(QString fileName);
    static QString getExtensionPixmapName(QString fileName, QString prefix);
    static FileType getFileType(QString fileName, QString prefix);

    static long long getSystemsAvailableMemory();

    static void sleepMilliseconds(unsigned int milliseconds);

    // Compute the part per <ref> of <part> from <total>. Defaults to %
    static int partPer(long long part, long long total, uint ref = 100);

    static QString getFileHash(const QString& filePath);

    // Human-friendly list of forbidden chars for New Remote Folder
    static const QLatin1String FORBIDDEN_CHARS;
    // Forbidden chars PCRE
    static const QRegularExpression FORBIDDEN_CHARS_RX;
    // Time to show the new remote folder input error in milliseconds
    static constexpr int ERROR_DISPLAY_TIME_MS = 10000; //10s in milliseconds

    static bool isNodeNameValid(const QString& name);
};

Q_DECLARE_METATYPE(Utilities::FileType)
Q_DECLARE_OPERATORS_FOR_FLAGS(Utilities::FileTypes)

template <class EnumType>
class EnumConversions
{
public:
    EnumConversions()
        : mMetaEnum(QMetaEnum::fromType<EnumType>())
    {
    }

    QString getString(EnumType type)
    {
        return QString::fromUtf8(mMetaEnum.valueToKey(static_cast<int>(type)));
    }

    EnumType  getEnum(const QString& typeAsString)
    {
        return static_cast<EnumType>(mMetaEnum.keyToValue(typeAsString.toUtf8().constData()));
    }

private:
    QMetaEnum mMetaEnum;
};

// This class encapsulates a MEGA node and adds useful information, like the origin
// of the transfer.
class WrappedNode
{
public:
    // Enum used to record origin of transfer
    enum TransferOrigin {
        FROM_UNKNOWN   = 0,
        FROM_APP       = 1,
        FROM_WEBSERVER = 2,
    };

    // Constructor with origin and pointer to MEGA node. Default to unknown/nullptr
    // @param undelete Indicates a special request for a node that has been completely deleted
    // (even from Rubbish Bin); allowed only for accounts with PRO level
    WrappedNode(TransferOrigin from = WrappedNode::TransferOrigin::FROM_UNKNOWN,
                mega::MegaNode* node = nullptr,
                bool undelete = false);

    // Destructor
    ~WrappedNode()
    {
        // MEGA node should be deleted when this is deleted.
        delete mNode;
    }

    // Get the transfer origin
    WrappedNode::TransferOrigin getTransferOrigin()
    {
        return mTransfersFrom;
    }

    // Get the wrapped MEGA node pointer
    mega::MegaNode* getMegaNode()
    {
        return mNode;
    }

    bool getUndelete() const
    {
        return mUndelete;
    }

private:
    // Keep track of transfer origin
    WrappedNode::TransferOrigin  mTransfersFrom;

    // Wrapped MEGA node
    mega::MegaNode* mNode;

    bool mUndelete;
};

Q_DECLARE_METATYPE(QQueue<WrappedNode*>)

#endif // UTILITIES_H
