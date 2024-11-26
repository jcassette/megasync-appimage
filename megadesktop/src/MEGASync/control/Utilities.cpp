#include "Utilities.h"

#include "Preferences.h"
#include "MegaApplication.h"
#include "gzjoin.h"
#include "Platform.h"

#include <QApplication>
#include <QImageReader>
#include <QDirIterator>
#include <QTextStream>
#include <QDateTime>
#include <QDesktopWidget>
#include <QScreen>
#include <QCryptographicHash>
#include <MegaApiSynchronizedRequest.h>

#include <iostream>

#ifndef WIN32
#include "megaapi.h"
#include <utime.h>
#else
#include <windows.h>
#include <shellapi.h>
#endif

using namespace std;
using namespace mega;

namespace
{
constexpr char AVATARS_EXTENSION_FILTER[] = "*.jpg";
constexpr int MSEC_IN_1_SEC = 1000;
constexpr int HOURS_IN_1_DAY = 24;
constexpr int SECS_IN_1_MIN = 60;
constexpr int SECS_IN_1_HOUR = 3600;
constexpr int SECS_IN_1_DAY = 86400;
constexpr int SECS_IN_1_MONTH = 2592000;
constexpr int SECS_IN_1_YEAR = 31536000;
}

QHash<QString, QString> Utilities::extensionIcons;
QHash<QString, Utilities::FileType> Utilities::fileTypes;
QHash<QString, QString> Utilities::languageNames;

std::unique_ptr<ThreadPool> ThreadPoolSingleton::instance = nullptr;

const QString Utilities::SUPPORT_URL = QString::fromUtf8("https://mega.nz/contact");
const QString Utilities::BACKUP_CENTER_URL = QString::fromLatin1("mega://#fm/devices");
const QString Utilities::SYNC_SUPPORT_URL = QString::fromLatin1("https://help.mega.io/installs-apps/desktop-syncing/sync-v2");

const long long KB = 1024;
const long long MB = 1024 * KB;
const long long GB = 1024 * MB;
const long long TB = 1024 * GB;

// Human-friendly list of forbidden chars for New Remote Folder
const QLatin1String Utilities::FORBIDDEN_CHARS("\\ / : \" * < > \? |");
// Forbidden chars PCRE using a capture list: [\\/:"\*<>?|]
const QRegularExpression Utilities::FORBIDDEN_CHARS_RX(QLatin1String("[\\\\/:\"*<>\?|]"));

const qint64 FILE_READ_BUFFER_SIZE = 8192;


void Utilities::initializeExtensions()
{
    extensionIcons[QString::fromLatin1("3ds")] = extensionIcons[QString::fromLatin1("3dm")]  = extensionIcons[QString::fromLatin1("max")] =
                            extensionIcons[QString::fromLatin1("obj")]  = QString::fromLatin1("3D.png");

    extensionIcons[QString::fromLatin1("aep")] = extensionIcons[QString::fromLatin1("aet")]  = QString::fromLatin1("aftereffects.png");

    extensionIcons[QString::fromLatin1("mp3")] = extensionIcons[QString::fromLatin1("wav")]  = extensionIcons[QString::fromLatin1("3ga")]  =
                            extensionIcons[QString::fromLatin1("aif")]  = extensionIcons[QString::fromLatin1("aiff")] =
                            extensionIcons[QString::fromLatin1("flac")] = extensionIcons[QString::fromLatin1("iff")]  = extensionIcons[QString::fromLatin1("ogg")] =
                            extensionIcons[QString::fromLatin1("m4a")]  = extensionIcons[QString::fromLatin1("wma")]  =  QString::fromLatin1("audio.png");

    extensionIcons[QString::fromLatin1("dxf")] = extensionIcons[QString::fromLatin1("dwg")] =  QString::fromLatin1("cad.png");


    extensionIcons[QString::fromLatin1("zip")] = extensionIcons[QString::fromLatin1("rar")] = extensionIcons[QString::fromLatin1("tgz")]  =
                            extensionIcons[QString::fromLatin1("gz")]  = extensionIcons[QString::fromLatin1("bz2")]  =
                            extensionIcons[QString::fromLatin1("tbz")] = extensionIcons[QString::fromLatin1("tar")]  =
                            extensionIcons[QString::fromLatin1("7z")]  = extensionIcons[QString::fromLatin1("sitx")] =  QString::fromLatin1("compressed.png");

    extensionIcons[QString::fromLatin1("sql")] = extensionIcons[QString::fromLatin1("accdb")] = extensionIcons[QString::fromLatin1("db")]  =
                            extensionIcons[QString::fromLatin1("dbf")]  = extensionIcons[QString::fromLatin1("mdb")]  =
                            extensionIcons[QString::fromLatin1("pdb")] = QString::fromLatin1("web_lang.png");

    extensionIcons[QString::fromLatin1("folder")] = QString::fromLatin1("folder.png");

    extensionIcons[QString::fromLatin1("xls")] = extensionIcons[QString::fromLatin1("xlsx")] = extensionIcons[QString::fromLatin1("xlt")]  =
                            extensionIcons[QString::fromLatin1("xltm")]  = QString::fromLatin1("excel.png");

    extensionIcons[QString::fromLatin1("exe")] = extensionIcons[QString::fromLatin1("com")] = extensionIcons[QString::fromLatin1("bin")]  =
                            extensionIcons[QString::fromLatin1("apk")]  = extensionIcons[QString::fromLatin1("app")]  =
                             extensionIcons[QString::fromLatin1("msi")]  = extensionIcons[QString::fromLatin1("cmd")]  =
                            extensionIcons[QString::fromLatin1("gadget")] = QString::fromLatin1("executable.png");

    extensionIcons[QString::fromLatin1("fnt")] = extensionIcons[QString::fromLatin1("otf")] = extensionIcons[QString::fromLatin1("ttf")]  =
                            extensionIcons[QString::fromLatin1("fon")]  = QString::fromLatin1("font.png");

    extensionIcons[QString::fromLatin1("gif")] = extensionIcons[QString::fromLatin1("tiff")]  = extensionIcons[QString::fromLatin1("tif")]  =
                            extensionIcons[QString::fromLatin1("bmp")]  = extensionIcons[QString::fromLatin1("png")] =
                            extensionIcons[QString::fromLatin1("tga")]  = QString::fromLatin1("image.png");

    extensionIcons[QString::fromLatin1("ai")] = extensionIcons[QString::fromLatin1("ait")] = QString::fromLatin1("illustrator.png");
    extensionIcons[QString::fromLatin1("jpg")] = extensionIcons[QString::fromLatin1("jpeg")] = extensionIcons[QString::fromLatin1("heic")] =
                            extensionIcons[QString::fromLatin1("webp")] = QString::fromLatin1("image.png");
    extensionIcons[QString::fromLatin1("indd")] = QString::fromLatin1("indesign.png");

    extensionIcons[QString::fromLatin1("jar")] = extensionIcons[QString::fromLatin1("java")]  = extensionIcons[QString::fromLatin1("class")]  = QString::fromLatin1("web_data.png");

    extensionIcons[QString::fromLatin1("pdf")] = QString::fromLatin1("pdf.png");
    extensionIcons[QString::fromLatin1("abr")] = extensionIcons[QString::fromLatin1("psb")]  = extensionIcons[QString::fromLatin1("psd")]  =
                            QString::fromLatin1("photoshop.png");

    extensionIcons[QString::fromLatin1("pps")] = extensionIcons[QString::fromLatin1("ppt")]  = extensionIcons[QString::fromLatin1("pptx")] = QString::fromLatin1("powerpoint.png");

    extensionIcons[QString::fromLatin1("prproj")] = extensionIcons[QString::fromLatin1("ppj")]  = QString::fromLatin1("premiere.png");

    extensionIcons[QString::fromLatin1("3fr")] = extensionIcons[QString::fromLatin1("arw")]  = extensionIcons[QString::fromLatin1("bay")]  =
                            extensionIcons[QString::fromLatin1("cr2")]  = extensionIcons[QString::fromLatin1("dcr")] =
                            extensionIcons[QString::fromLatin1("dng")] = extensionIcons[QString::fromLatin1("fff")]  =
                            extensionIcons[QString::fromLatin1("mef")] = extensionIcons[QString::fromLatin1("mrw")]  =
                            extensionIcons[QString::fromLatin1("nef")] = extensionIcons[QString::fromLatin1("pef")]  =
                            extensionIcons[QString::fromLatin1("rw2")] = extensionIcons[QString::fromLatin1("srf")]  =
                            extensionIcons[QString::fromLatin1("orf")] = extensionIcons[QString::fromLatin1("rwl")]  =
                            extensionIcons[QString::fromLatin1("ari")] = extensionIcons[QString::fromLatin1("braw")]  =
                            extensionIcons[QString::fromLatin1("crw")] = extensionIcons[QString::fromLatin1("cr3")]  =
                            extensionIcons[QString::fromLatin1("cap")] =
                            extensionIcons[QString::fromLatin1("dcs")] = extensionIcons[QString::fromLatin1("drf")]  =
                            extensionIcons[QString::fromLatin1("eip")] = extensionIcons[QString::fromLatin1("erf")]  =
                            extensionIcons[QString::fromLatin1("gpr")] = extensionIcons[QString::fromLatin1("iiq")]  =
                            extensionIcons[QString::fromLatin1("k25")] = extensionIcons[QString::fromLatin1("kdc")]  =
                            extensionIcons[QString::fromLatin1("mdc")] = extensionIcons[QString::fromLatin1("mos")]  =
                            extensionIcons[QString::fromLatin1("nrw")] = extensionIcons[QString::fromLatin1("obm")]  =
                            extensionIcons[QString::fromLatin1("ptx")] = extensionIcons[QString::fromLatin1("pxn")]  =
                            extensionIcons[QString::fromLatin1("r3d")] = extensionIcons[QString::fromLatin1("raf")]  =
                            extensionIcons[QString::fromLatin1("raw")] = extensionIcons[QString::fromLatin1("rwz")]  =
                            extensionIcons[QString::fromLatin1("sr2")] = extensionIcons[QString::fromLatin1("srw")]  =
                            extensionIcons[QString::fromLatin1("tif")] = extensionIcons[QString::fromLatin1("x3f")]  =
                            QString::fromLatin1("raw.png");

    extensionIcons[QString::fromLatin1("ods")]  = extensionIcons[QString::fromLatin1("ots")]  =
                            extensionIcons[QString::fromLatin1("gsheet")]  = extensionIcons[QString::fromLatin1("nb")] =
                            extensionIcons[QString::fromLatin1("xlr")] = QString::fromLatin1("spreadsheet.png");

    extensionIcons[QString::fromLatin1("torrent")] = QString::fromLatin1("torrent.png");
    extensionIcons[QString::fromLatin1("dmg")] = QString::fromLatin1("dmg.png");

    extensionIcons[QString::fromLatin1("txt")] = extensionIcons[QString::fromLatin1("rtf")]  = extensionIcons[QString::fromLatin1("ans")]  =
                            extensionIcons[QString::fromLatin1("ascii")]  = extensionIcons[QString::fromLatin1("log")] =
                            extensionIcons[QString::fromLatin1("odt")] = extensionIcons[QString::fromLatin1("wpd")]  =
                            QString::fromLatin1("text.png");

    extensionIcons[QString::fromLatin1("svgz")]  = extensionIcons[QString::fromLatin1("svg")]  =
                            extensionIcons[QString::fromLatin1("cdr")]  = extensionIcons[QString::fromLatin1("eps")] =
                            QString::fromLatin1("vector.png");

     extensionIcons[QString::fromLatin1("mkv")]  = extensionIcons[QString::fromLatin1("webm")]  =
                            extensionIcons[QString::fromLatin1("avi")]  = extensionIcons[QString::fromLatin1("mp4")] =
                            extensionIcons[QString::fromLatin1("m4v")] = extensionIcons[QString::fromLatin1("mpg")]  =
                            extensionIcons[QString::fromLatin1("mpeg")] = extensionIcons[QString::fromLatin1("mov")]  =
                            extensionIcons[QString::fromLatin1("3g2")] = extensionIcons[QString::fromLatin1("3gp")]  =
                            extensionIcons[QString::fromLatin1("asf")] = extensionIcons[QString::fromLatin1("wmv")]  =
                            extensionIcons[QString::fromLatin1("flv")] = extensionIcons[QString::fromLatin1("vob")] =
                            QString::fromLatin1("video.png");

     extensionIcons[QString::fromLatin1("html")]  = extensionIcons[QString::fromLatin1("xml")] = extensionIcons[QString::fromLatin1("shtml")]  =
                            extensionIcons[QString::fromLatin1("dhtml")] = extensionIcons[QString::fromLatin1("js")] =
                            extensionIcons[QString::fromLatin1("css")]  = QString::fromLatin1("web_data.png");

     extensionIcons[QString::fromLatin1("php")]  = extensionIcons[QString::fromLatin1("php3")]  =
                            extensionIcons[QString::fromLatin1("php4")]  = extensionIcons[QString::fromLatin1("php5")] =
                            extensionIcons[QString::fromLatin1("phtml")] = extensionIcons[QString::fromLatin1("inc")]  =
                            extensionIcons[QString::fromLatin1("asp")] = extensionIcons[QString::fromLatin1("pl")]  =
                            extensionIcons[QString::fromLatin1("cgi")] = extensionIcons[QString::fromLatin1("py")]  =
                            QString::fromLatin1("web_lang.png");

     extensionIcons[QString::fromLatin1("doc")]  = extensionIcons[QString::fromLatin1("docx")] = extensionIcons[QString::fromLatin1("dotx")]  =
                            extensionIcons[QString::fromLatin1("wps")] = QString::fromLatin1("word.png");

     extensionIcons[QString::fromLatin1("odt")]  = extensionIcons[QString::fromLatin1("ods")] = extensionIcons[QString::fromLatin1("odp")]  =
                            extensionIcons[QString::fromLatin1("odb")] = extensionIcons[QString::fromLatin1("odg")] = QString::fromLatin1("openoffice.png");

     extensionIcons[QString::fromLatin1("sketch")] = QString::fromLatin1("sketch.png");
     extensionIcons[QString::fromLatin1("xd")] = QString::fromLatin1("experiencedesign.png");
     extensionIcons[QString::fromLatin1("pages")] = QString::fromLatin1("pages.png");
     extensionIcons[QString::fromLatin1("numbers")] = QString::fromLatin1("numbers.png");
     extensionIcons[QString::fromLatin1("key")] = QString::fromLatin1("keynote.png");
}

void Utilities::initializeFileTypes()
{
    fileTypes[getExtensionPixmapName(QLatin1String("a.wav"), QString())]
            = FileType::TYPE_AUDIO;
    fileTypes[getExtensionPixmapName(QLatin1String("a.mkv"), QString())]
            = FileType::TYPE_VIDEO;
    fileTypes[getExtensionPixmapName(QLatin1String("a.tar"), QString())]
            = FileType::TYPE_ARCHIVE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.torrent"), QString())]
            = FileType::TYPE_ARCHIVE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.dmg"), QString())]
            = FileType::TYPE_ARCHIVE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.xd"), QString())]
            = FileType::TYPE_ARCHIVE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.sketch"), QString())]
            = FileType::TYPE_ARCHIVE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.txt"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.odt"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.pdf"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.doc"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.ods"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.odt"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.ppt"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.pages"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.numbers"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.key"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.xml"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.xls"), QString())]
            = FileType::TYPE_DOCUMENT;
    fileTypes[getExtensionPixmapName(QLatin1String("a.png"), QString())]
            = FileType::TYPE_IMAGE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.jpg"), QString())]
            = FileType::TYPE_IMAGE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.ai"), QString())]
            = FileType::TYPE_IMAGE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.abr"), QString())]
            = FileType::TYPE_IMAGE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.3fr"), QString())]
            = FileType::TYPE_IMAGE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.svg"), QString())]
            = FileType::TYPE_IMAGE;
    fileTypes[getExtensionPixmapName(QLatin1String("a.bin"), QString())]
            = FileType::TYPE_OTHER;
}


void Utilities::queueFunctionInAppThread(std::function<void()> fun) {
   QObject temporary;
   QObject::connect(&temporary, &QObject::destroyed, qApp, std::move(fun), Qt::QueuedConnection);
}

void Utilities::queueFunctionInObjectThread(QObject* object, std::function<void()> fun) {
   QObject temporary;
   QObject::connect(&temporary, &QObject::destroyed, object, std::move(fun), Qt::QueuedConnection);
}

void Utilities::getFolderSize(QString folderPath, long long *size)
{
    if (!folderPath.size())
    {
        return;
    }

    QDir dir(folderPath);
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    for (int i = 0; i < entries.size(); i++)
    {
        QFileInfo info = entries[i];
        if (info.isFile())
        {
            (*size) += info.size();
        }
        else if (info.isDir())
        {
            getFolderSize(info.absoluteFilePath(), size);
        }
    }
}

qreal Utilities::getDevicePixelRatio()
{
    return qApp->testAttribute(Qt::AA_UseHighDpiPixmaps) ? qApp->devicePixelRatio() : 1.0;
}

QString Utilities::getExtensionPixmapName(QString fileName, QString prefix)
{
    if (extensionIcons.isEmpty())
    {
        initializeExtensions();
        initializeFileTypes();
    }

    QFileInfo f(fileName);
    if (extensionIcons.contains(f.suffix().toLower()))
    {
        return QString(extensionIcons[f.suffix().toLower()]).insert(0, prefix);
    }
    else
    {
        return prefix + QString::fromLatin1("generic.png");
    }
}

Utilities::FileType Utilities::getFileType(QString fileName, QString prefix)
{
    auto extensionPixmapName = getExtensionPixmapName(fileName, prefix);
    return fileTypes.value(extensionPixmapName,FileType::TYPE_OTHER);
}

QString Utilities::languageCodeToString(QString code)
{
    if (languageNames.isEmpty())
    {
        languageNames[QString::fromLatin1("ar")] = QString::fromUtf8("العربية"); // arabic
        languageNames[QString::fromLatin1("de")] = QString::fromUtf8("Deutsch");
        languageNames[QString::fromLatin1("en")] = QString::fromUtf8("English");
        languageNames[QString::fromLatin1("es")] = QString::fromUtf8("Español");
        languageNames[QString::fromLatin1("fr")] = QString::fromUtf8("Français");
        languageNames[QString::fromLatin1("id")] = QString::fromUtf8("Bahasa Indonesia");
        languageNames[QString::fromLatin1("it")] = QString::fromUtf8("Italiano");
        languageNames[QString::fromLatin1("ja")] = QString::fromUtf8("日本語"); // japanese
        languageNames[QString::fromLatin1("ko")] = QString::fromUtf8("한국어"); // korean
        languageNames[QString::fromLatin1("nl")] = QString::fromUtf8("Nederlands");
        languageNames[QString::fromLatin1("pl")] = QString::fromUtf8("Polski");
        languageNames[QString::fromLatin1("pt")] = QString::fromUtf8("Português");
        languageNames[QString::fromLatin1("ro")] = QString::fromUtf8("Română");
        languageNames[QString::fromLatin1("ru")] = QString::fromUtf8("Pусский");
        languageNames[QString::fromLatin1("th")] = QString::fromUtf8("ภาษาไทย"); // thai
        languageNames[QString::fromLatin1("vi")] = QString::fromUtf8("Tiếng Việt");
        languageNames[QString::fromLatin1("zh_CN")] = QString::fromUtf8("简体中文");
        languageNames[QString::fromLatin1("zh_TW")] = QString::fromUtf8("中文繁體");


        // Currently unsupported
        // languageNames[QString::fromLatin1("mi")] = QString::fromUtf8("Māori");
        // languageNames[QString::fromLatin1("ca")] = QString::fromUtf8("Català");
        // languageNames[QString::fromLatin1("eu")] = QString::fromUtf8("Euskara");
        // languageNames[QString::fromLatin1("af")] = QString::fromUtf8("Afrikaans");
        // languageNames[QString::fromLatin1("no")] = QString::fromUtf8("Norsk");
        // languageNames[QString::fromLatin1("bs")] = QString::fromUtf8("Bosanski");
        // languageNames[QString::fromLatin1("da")] = QString::fromUtf8("Dansk");
        // languageNames[QString::fromLatin1("el")] = QString::fromUtf8("ελληνικά");
        // languageNames[QString::fromLatin1("lt")] = QString::fromUtf8("Lietuvos");
        // languageNames[QString::fromLatin1("lv")] = QString::fromUtf8("Latviešu");
        // languageNames[QString::fromLatin1("mk")] = QString::fromUtf8("македонски");
        // languageNames[QString::fromLatin1("hi")] = QString::fromUtf8("हिंदी");
        // languageNames[QString::fromLatin1("ms")] = QString::fromUtf8("Bahasa Malaysia");
        // languageNames[QString::fromLatin1("cy")] = QString::fromUtf8("Cymraeg");
        // languageNames[QString::fromLatin1("ee")] = QString::fromUtf8("Eesti");
        // languageNames[QString::fromLatin1("fa")] = QString::fromUtf8("فارسی");
        // languageNames[QString::fromLatin1("hr")] = QString::fromUtf8("Hrvatski");
        // languageNames[QString::fromLatin1("ka")] = QString::fromUtf8("ქართული");
        // languageNames[QString::fromLatin1("cs")] = QString::fromUtf8("Čeština");
        // languageNames[QString::fromLatin1("sk")] = QString::fromUtf8("Slovenský");
        // languageNames[QString::fromLatin1("sl")] = QString::fromUtf8("Slovenščina");
        // languageNames[QString::fromLatin1("hu")] = QString::fromUtf8("Magyar");
        // languageNames[QString::fromLatin1("fi")] = QString::fromUtf8("Suomi");
        // languageNames[QString::fromLatin1("sr")] = QString::fromUtf8("српски");
        // languageNames[QString::fromLatin1("sv")] = QString::fromUtf8("Svenska");
        // languageNames[QString::fromLatin1("bg")] = QString::fromUtf8("български");
        // languageNames[QString::fromLatin1("he")] = QString::fromUtf8("עברית");
        // languageNames[QString::fromLatin1("tr")] = QString::fromUtf8("Türkçe");
        // languageNames[QString::fromLatin1("tl")] = QString::fromUtf8("Tagalog");
        // languageNames[QString::fromLatin1("uk")] = QString::fromUtf8("Українська");
        // languageNames[QString::fromLatin1("pt")] = QString::fromUtf8("Português");


    }
    return languageNames.value(code);
}


struct IconCache
{
    std::map<QString, QIcon> mIcons;

    QIcon& getDirect(QString resourceName)
    {
        auto i = mIcons.find(resourceName);
        if (i == mIcons.end())
        {
            auto pair = mIcons.emplace(resourceName, QIcon());
            i = pair.first;
            i->second.addFile(resourceName, QSize(), QIcon::Normal, QIcon::Off);
        }
        return i->second;
    }

    QIcon& getByExtension(const QString &fileName, const QString &prefix)
    {
        return getDirect(Utilities::getExtensionPixmapName(fileName, prefix));
    }
};

IconCache gIconCache;

QString Utilities::getExtensionPixmapNameSmall(QString fileName)
{
    return getExtensionPixmapName(fileName, QString::fromLatin1(":/images/small_"));
}

QString Utilities::getExtensionPixmapNameMedium(QString fileName)
{
    return getExtensionPixmapName(fileName, QString::fromLatin1(":/images/drag_"));
}

double Utilities::toDoubleInUnit(unsigned long long bytes, unsigned long long unit)
{
    double decimalMultiplier = 100.0;
    double multipliedValue = decimalMultiplier * static_cast<double>(bytes);
    return static_cast<int>(round(multipliedValue / static_cast<double>(unit))) / decimalMultiplier;
}

QString Utilities::getTimeFormat(const TimeInterval &interval)
{
    QString timeFormat;

    if (interval.days)
    {
        timeFormat = QCoreApplication::translate("Utilities", "[DAYS] [HOURS]");
        if (!interval.hours)
        {
            timeFormat.remove(QString::fromLatin1("[HOURS]"));
        }
    }
    else if (interval.hours)
    {
        timeFormat = QCoreApplication::translate("Utilities", "[HOURS] [MINUTES]");
        if (!interval.minutes)
        {
            timeFormat.remove(QString::fromLatin1("[MINUTES]"));
        }
    }
    else
    {
        timeFormat = QCoreApplication::translate("Utilities", "[MINUTES] [SECONDS]");
        if (!interval.minutes)
        {
            timeFormat.remove(QString::fromLatin1("[MINUTES]"));
        }
        if (!interval.useSecondPrecision)
        {
            timeFormat.remove(QString::fromLatin1("[SECONDS]"));
        }
    }

    return timeFormat.trimmed();
}

QString Utilities::filledTimeString(const QString &timeFormat, const TimeInterval &interval, bool color)
{
    QString daysFormat = QCoreApplication::translate("Utilities", "%1 [A]d[/A]");
    QString hoursFormat = QCoreApplication::translate("Utilities", "%1 [A]h[/A]");
    QString minutesFormat = QCoreApplication::translate("Utilities", "%1 [A]m[/A]");
    QString secondsFormat = QCoreApplication::translate("Utilities", "%1 [A]s[/A]");

    QString timeString = timeFormat;

    QString timeCountStr = daysFormat.arg(interval.days, 2, 10, QLatin1Char('0'));
    timeString.replace(QString::fromLatin1("[DAYS]"), timeCountStr);

    timeCountStr = hoursFormat.arg(interval.hours, 2, 10, QLatin1Char('0'));
    timeString.replace(QString::fromLatin1("[HOURS]"), timeCountStr);

    timeCountStr = minutesFormat.arg(interval.minutes, 2, 10, QLatin1Char('0'));
    timeString.replace(QString::fromLatin1("[MINUTES]"), timeCountStr);

    timeCountStr = secondsFormat.arg(interval.seconds, 2, 10, QLatin1Char('0'));
    timeString.replace(QString::fromLatin1("[SECONDS]"), timeCountStr);

    QString colorString = (color ? QLatin1String("color:#777777;") : QString());
    QString styleStartTag = QString::fromUtf8("<span style=\"%1 text-decoration:none;\">").arg(colorString);
    QString styleEndTag = QString::fromLatin1("</span>");

    timeString.replace(QString::fromLatin1("[A]"),  styleStartTag);
    timeString.replace(QString::fromLatin1("[/A]"), styleEndTag);

    return cleanedTimeString(timeString);
}

QString Utilities::cleanedTimeString(const QString &timeString)
{
    QString cleanedStr = timeString;
    if(!cleanedStr.isEmpty() && cleanedStr.at(0) == QLatin1Char('0'))
    {
        cleanedStr.replace(0, 1, QLatin1Char(' '));
    }
    return cleanedStr;
}

long long Utilities::getNearestUnit(long long bytes)
{
    if (bytes >= TB)
    {
        return TB;
    }
    else if (bytes >= GB)
    {
        return GB;
    }
    else if (bytes >= MB)
    {
        return MB;
    }
    else if (bytes >= KB)
    {
        return KB;
    }
    return 1;
}

QIcon Utilities::getCachedPixmap(QString fileName)
{
    return gIconCache.getDirect(fileName);
}

QIcon Utilities::getExtensionPixmapSmall(QString fileName)
{
    return gIconCache.getDirect(getExtensionPixmapNameSmall(fileName));
}

QIcon Utilities::getExtensionPixmapMedium(QString fileName)
{
    return gIconCache.getDirect(getExtensionPixmapNameMedium(fileName));
}

QString Utilities::getAvatarPath(QString email)
{
    if (!email.size())
    {
        return QString();
    }

    QString avatarsPath = QString::fromUtf8("%1/avatars/%2.jpg")
            .arg(Preferences::instance()->getDataPath()).arg(email);
    return QDir::toNativeSeparators(avatarsPath);
}

void Utilities::removeAvatars()
{   
    const QString avatarsPath = QString::fromUtf8("%1/avatars/").arg(Preferences::instance()->getDataPath());
    QDir avatarsDirectory(avatarsPath);
    avatarsDirectory.setNameFilters(QStringList() << QString::fromUtf8(::AVATARS_EXTENSION_FILTER));
    avatarsDirectory.setFilter(QDir::Files);
    const QStringList avatars = avatarsDirectory.entryList();
    for(const QString &avatar: avatars)
    {
        avatarsDirectory.remove(avatar);
    }
}

bool Utilities::removeRecursively(QString path)
{
    if (!path.size())
    {
        return false;
    }

    QDir dir(path);
    bool success = false;
    QString qpath = QDir::toNativeSeparators(dir.absolutePath());
    MegaApi::removeRecursively(qpath.toUtf8().constData());
    success = dir.rmdir(dir.absolutePath());
    return success;
}

void Utilities::copyRecursively(QString srcPath, QString dstPath)
{
    if (!srcPath.size() || !dstPath.size())
    {
        return;
    }

    QFileInfo source(srcPath);
    if (!source.exists())
    {
        return;
    }

    if (srcPath == dstPath)
    {
        return;
    }

    QFile dst(dstPath);
    if (dst.exists())
    {
        return;
    }

    if (source.isFile())
    {
        QFile src(srcPath);
        src.copy(dstPath);
#ifndef _WIN32
       QFileInfo info(src);
       time_t t = info.lastModified().toTime_t();
       struct utimbuf times = { t, t };
       utime(dstPath.toUtf8().constData(), &times);
#endif
    }
    else if (source.isDir())
    {
        QDir dstDir(dstPath);
        dstDir.mkpath(QString::fromLatin1("."));
        QDirIterator di(srcPath, QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
        while (di.hasNext())
        {
            di.next();
            if (!di.fileInfo().isSymLink())
            {
                copyRecursively(di.filePath(), dstPath + QDir::separator() + di.fileName());
            }
        }
    }
}

QString Utilities::getTimeString(long long secs, bool secondPrecision, bool color)
{
    TimeInterval interval(secs, secondPrecision);
    return filledTimeString(getTimeFormat(interval), interval, color);
}

QString Utilities::getAddedTimeString(long long secs)
{
    if (secs < 2)
    {
        return QCoreApplication::translate("Utilities", "Added just now");
    }
    else if (secs < SECS_IN_1_MIN)
    {
        const int secsAsInt = static_cast<int>(secs);
        return QCoreApplication::translate("Utilities", "Added %n second ago", "", secsAsInt);
    }
    else if (secs < SECS_IN_1_HOUR)
    {
        const int minutes = static_cast<int>(secs/SECS_IN_1_MIN);
        return QCoreApplication::translate("Utilities", "Added %n minute ago", "", minutes);
    }
    else if (secs < SECS_IN_1_DAY)
    {
        const int hours = static_cast<int>(secs/SECS_IN_1_HOUR);
        return QCoreApplication::translate("Utilities", "Added %n hour ago", "", hours);
    }
    else if (secs < SECS_IN_1_MONTH)
    {
        const int days = static_cast<int>(secs/SECS_IN_1_DAY);
        return QCoreApplication::translate("Utilities", "Added %n day ago", "", days);
    }
    else if (secs < SECS_IN_1_YEAR)
    {
        const int months = static_cast<int>(secs/SECS_IN_1_MONTH);
        return QCoreApplication::translate("Utilities", "Added %n month ago", "", months);
    }
    // We might not need century precision... give years.
    else
    {
        const int years = static_cast<int>(secs/SECS_IN_1_YEAR);
        return QCoreApplication::translate("Utilities", "Added %n year ago", "", years);
    }
}

QString Utilities::getSizeString(long long bytes)
{
    QString language = ((MegaApplication*)qApp)->getCurrentLanguageCode();
    QLocale locale(language);
    
    if (bytes >= TB)
    {
        return locale.toString(toDoubleInUnit(bytes, TB)) + QString::fromLatin1(" ")
                + QCoreApplication::translate("Utilities", "TB");
    }

    if (bytes >= GB)
    {
        return locale.toString(toDoubleInUnit(bytes, GB)) + QString::fromLatin1(" ")
                + QCoreApplication::translate("Utilities", "GB");
    }

    if (bytes >= MB)
    {
        return locale.toString(toDoubleInUnit(bytes, MB)) + QString::fromLatin1(" ")
                + QCoreApplication::translate("Utilities", "MB");
    }

    if (bytes >= KB)
    {
        return locale.toString(toDoubleInUnit(bytes, KB)) + QString::fromLatin1(" ")
                + QCoreApplication::translate("Utilities", "KB");
    }

    return locale.toString(toDoubleInUnit(bytes, 1)) + QString::fromLatin1(" ")
                    + QCoreApplication::translate("Utilities", "Bytes");
}

QString Utilities::getSizeStringLocalized(qint64 bytes)
{
    auto baseUnitSize = Platform::getInstance()->getBaseUnitsSize();

    static const QVector<QPair<QString, qint64>> unitsSize{
        {QLatin1String("TB"), static_cast<qint64>(std::pow(baseUnitSize, 4))},
        {QLatin1String("GB"), static_cast<qint64>(std::pow(baseUnitSize, 3))},
        {QLatin1String("MB"), static_cast<qint64>(std::pow(baseUnitSize, 2))},
        {QLatin1String("KB"), baseUnitSize},
        {QLatin1String("Bytes"), 1}
    };

    auto foundIt = std::find_if(unitsSize.constBegin(), unitsSize.constEnd(), [&bytes](const QPair<QString, qint64>& pair){
        return bytes >= pair.second;
    });

    QString language = ((MegaApplication*)qApp)->getCurrentLanguageCode();
    QLocale locale(language);

    if (foundIt != unitsSize.constEnd())
    {
        return locale.toString(toDoubleInUnit(bytes, foundIt->second)) + QString::fromLatin1(" ")
                + QCoreApplication::translate("Utilities", foundIt->first.toUtf8().constData());
    }
    else
    {
        return locale.toString(toDoubleInUnit(bytes, 1)) + QString::fromLatin1(" ")
                + QCoreApplication::translate("Utilities", "Bytes");
    }
}

int Utilities::toNearestUnit(long long bytes)
{
    unsigned long long nearestUnit = getNearestUnit(bytes);
    double inNearestUnit = toDoubleInUnit(bytes, nearestUnit);
    return static_cast<int>(inNearestUnit);
}

QString Utilities::getTranslatedSeparatorTemplate()
{
    return QCoreApplication::translate("Utilities", "%1/%2");
}

Utilities::ProgressSize Utilities::getProgressSizes(long long transferredBytes, long long totalBytes)
{
    ProgressSize sizes;
    Q_ASSERT(totalBytes >= transferredBytes);

    if (transferredBytes >= 0 && totalBytes >= 0)
    {    
        QString language = ((MegaApplication*)qApp)->getCurrentLanguageCode();
        QLocale locale(language);

        //Init on 1 for Bytes
        unsigned long long sizeToCompare(0);
        QString Units;

        if (totalBytes >= TB)
        {
            sizeToCompare = TB;
            Units = QCoreApplication::translate("Utilities", "TB");
        }
        else if (totalBytes >= GB)
        {
            sizeToCompare = GB;
            Units = QCoreApplication::translate("Utilities", "GB");
        }
        else if (totalBytes >= MB)
        {
            sizeToCompare = MB;
            Units = QCoreApplication::translate("Utilities", "MB");
        }
        else if (totalBytes >= KB)
        {
            sizeToCompare = KB;
            Units = QCoreApplication::translate("Utilities", "KB");
        }
        else
        {
            sizeToCompare = 1;
            Units = QCoreApplication::translate("Utilities", "Bytes");
        }

        sizes.transferredBytes = locale.toString( ((int)((10 * transferredBytes) / sizeToCompare))/10.0);
        sizes.totalBytes = locale.toString( ((int)((10 * totalBytes) / sizeToCompare))/10.0);
        sizes.units = QStringLiteral(" ") + Units;
    }

    return sizes;
}

QString Utilities::createSimpleUsedString(long long usedData)
{
    return createSimpleUsedStringWithoutReplacement(usedData).arg(getSizeString(usedData));
}

QString Utilities::createSimpleUsedOfString(long long usedData, long long totalData)
{
    return QCoreApplication::translate("Utilities", "%1 of %2")
        .arg(getSizeString(usedData))
        .arg(getSizeString(totalData));
}

QString Utilities::createSimpleUsedStringWithoutReplacement(long long usedData)
{
    return QCoreApplication::translate("Utilities", "%1 used", "", toNearestUnit(usedData));
}

QString Utilities::createCompleteUsedString(long long usedData, long long totalData, int percentage)
{
    return QCoreApplication::translate("Utilities", "%1 (%2%) of %3 used", "", toNearestUnit(usedData)).arg(
            getSizeString(usedData),
            QString::number(percentage),
            getSizeString(totalData)
            );
}

QString Utilities::extractJSONString(QString json, QString name)
{
    QString pattern = name + QString::fromUtf8("\":\"");
    int pos = json.indexOf(pattern);
    if (pos < 0)
    {
        return QString();
    }

    int end = json.indexOf(QString::fromUtf8("\""), pos + pattern.size());
    if (end < 0)
    {
        return QString();
    }

    return json.mid(pos + pattern.size(), end - pos - pattern.size());
}

QStringList Utilities::extractJSONStringList(const QString& json, const QString& name)
{
    QStringList resultList;

    QString pattern = name + QString::fromUtf8("\":[\"");
    int startPos = json.indexOf(pattern);
    if (startPos < 0)
    {
        return resultList;
    }

    startPos += pattern.size(); // Move to the beginning of the first string

    int endPos = json.indexOf(QString::fromUtf8("\"]"), startPos);
    if (endPos < 0)
    {
        return resultList;
    }

    QString substr = json.mid(startPos, endPos - startPos);
    QStringList parts = substr.remove(QString::fromUtf8("\"")).split(QString::fromUtf8(","));

    // Trim whitespace from each part and add it to the result list
    for (const QString& part : parts)
    {
        resultList.append(part.trimmed());
    }

    return resultList;
}

long long Utilities::extractJSONNumber(QString json, QString name)
{
    QString pattern = name + QString::fromUtf8("\":");
    int pos = json.indexOf(pattern);
    if (pos < 0)
    {
        return 0;
    }

    int end = pos + pattern.size();
    int count = 0;
    while (json[end].isDigit())
    {
        end++;
        count++;
    }

    return json.mid(pos + pattern.size(), count).toLongLong();
}

QString Utilities::getDefaultBasePath()
{
        QStringList defaultPaths;
#ifdef WIN32
        defaultPaths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
        if (defaultPaths.size())
        {
            return defaultPaths.at(0);
        }

        defaultPaths = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        if (defaultPaths.size())
        {
            return defaultPaths.at(0);
        }
#else
        defaultPaths = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        if (defaultPaths.size())
        {
            return defaultPaths.at(0);
        }

        defaultPaths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
        if (defaultPaths.size())
        {
            return defaultPaths.at(0);
        }
#endif
        defaultPaths = QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
        if (defaultPaths.size())
        {
            return defaultPaths.at(0);
        }

        defaultPaths = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation);
        if (defaultPaths.size())
        {
            return defaultPaths.at(0);
        }

    QDir dataDir = QDir(Preferences::instance()->getDataPath());
    QString rootPath = QDir::toNativeSeparators(dataDir.rootPath());
    if (rootPath.size() && rootPath.at(rootPath.size() - 1) == QDir::separator())
    {
        rootPath.resize(rootPath.size() - 1);
        return rootPath;
    }
    return QString();
}

void Utilities::getPROurlWithParameters(QString &url)
{
    auto preferences = Preferences::instance();
    MegaApi *megaApi = ((MegaApplication *)qApp)->getMegaApi();

    if (!preferences || !megaApi)
    {
        return;
    }

    QString userAgent = QString::fromUtf8(QUrl::toPercentEncoding(QString::fromUtf8(megaApi->getUserAgent())));
    url.append(QString::fromUtf8("/uao=%1").arg(userAgent));

    MegaHandle aff;
    int affType;
    long long timestamp;
    preferences->getLastHandleInfo(aff, affType, timestamp);

    if (aff != INVALID_HANDLE)
    {
        char *base64aff = MegaApi::handleToBase64(aff);
        url.append(QString::fromUtf8("/aff=%1/aff_time=%2/aff_type=%3").arg(QString::fromUtf8(base64aff))
                                                                       .arg(timestamp / MSEC_IN_1_SEC)
                                                                       .arg(affType));
        delete [] base64aff;
    }
}

QString Utilities::joinLogZipFiles(MegaApi *megaApi, const QDateTime *timestampSince, QString appenHashReference)
{
    if (!megaApi)
    {
        return QString();
    }

    QDir logDir{MegaApplication::applicationDataPath().append(QString::fromUtf8("/") + LOGS_FOLDER_LEAFNAME_QSTRING)};
    if (logDir.exists())
    {
        QString fileFormat{QDir::separator() + QString::fromUtf8("%1%2%3")
                                                    .arg(QDateTime::currentDateTimeUtc().toString(QString::fromLatin1("yyMMdd_hhmmss")))
                                                    .arg(megaApi->getMyUser() ? QString::fromUtf8("_") + QString::fromUtf8(std::unique_ptr<MegaUser>(megaApi->getMyUser())->getEmail()) : QString::fromUtf8(""))
                                                    .arg(!appenHashReference.isEmpty() ? QString::fromUtf8("_") + appenHashReference : QString::fromUtf8(""))};

        QFileInfo joinLogsFile(logDir.absolutePath().append(fileFormat).append(QString::fromUtf8(".gz")));
#ifdef _WIN32
        FILE * pFile = nullptr;
        errno_t er = _wfopen_s(&pFile, joinLogsFile.absoluteFilePath().toStdWString().c_str(), L"a+b");
        if (er)
        {
            megaApi->log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error opening file for joining log zip files (%1) : %2")
                         .arg(er).arg(joinLogsFile.filePath()).toUtf8().constData());
            pFile = nullptr; //just in case
        }

#else
        FILE * pFile = fopen(joinLogsFile.absoluteFilePath().toUtf8().constData(), "a+b");
#endif
        if (!pFile)
        {
            std::cerr << "Error opening file for joining log zip files " << std::endl;
            megaApi->log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error opening file for joining log zip files: %1").arg(joinLogsFile.filePath()).toUtf8().constData());
            return QString();
        }

        unsigned long crc, tot;
        gzinit(&crc, &tot, pFile);

        QFileInfoList logFiles = logDir.entryInfoList(QStringList() << QString::fromUtf8("MEGAsync.[0-9]*.log"), QDir::Files);
        int nLogFiles = logFiles.count();

        std::sort(logFiles.begin(), logFiles.end(), [](const QFileInfo &v1, const QFileInfo &v2){
            return v1.fileName().remove(QRegExp(QString::fromUtf8("[^\\d]"))).toInt() > v2.fileName().remove(QRegExp(QString::fromUtf8("[^\\d]"))).toInt();} );

        foreach (QFileInfo i, logFiles)
        {
            --nLogFiles;

            if (timestampSince)
            {
                if ( i.lastModified() < *timestampSince && i.fileName() != QString::fromUtf8("MEGAsync.0.log")) //keep at least the last log
                {
                    continue;
                }
            }

            try
            {
#ifdef _WIN32
                gzcopy(i.absoluteFilePath().toStdWString().c_str(), nLogFiles, &crc, &tot, pFile);
#else
                gzcopy(i.absoluteFilePath().toUtf8().constData(), nLogFiles, &crc, &tot, pFile);
#endif
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error joining zip files for bug report " << e.what() << std::endl;
                megaApi->log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error joining zip files for bug report : %1")
                             .arg(QString::fromUtf8(e.what())).toUtf8().constData());

                fclose(pFile);
                QFile::remove(joinLogsFile.absoluteFilePath());
                return QString();
            }
        }

        fclose(pFile);
        return joinLogsFile.absoluteFilePath();
    }

    return QString();
}

void Utilities::adjustToScreenFunc(QPoint position, QWidget *what)
{
    const auto screen = QGuiApplication::screenAt(position);
    auto screenGeometry = screen->availableGeometry();
    if (screenGeometry.isValid())
    {
        int newx = what->x(), newy = what->y();
        if (what->x() < screenGeometry.x())
        {
            newx = screenGeometry.x();
        }
        if (what->y() < screenGeometry.y())
        {
            newy = screenGeometry.y();
        }
        if (what->x() + what->width() > screenGeometry.right())
        {
            newx = screenGeometry.right() - what->width();
        }
        if (what->y() + what->height() > screenGeometry.bottom())
        {
            newy = screenGeometry.bottom() - what->height();
        }
        if (newx != what->x() || newy != what->y())
        {
            what->move(newx, newy);
        }
    }
}

QString Utilities::minProPlanNeeded(std::shared_ptr<MegaPricing> pricing, long long usedStorage)
{
    if (!pricing)
    {
        return QString::fromUtf8("Pro");
    }

    int planNeeded = -1;
    int amountPlanNeeded = 0;
    int products = pricing->getNumProducts();
    for (int i = 0; i < products; i++)
    {
        //Skip business & non monthly plans to offer
        if (!pricing->isBusinessType(i) && pricing->getMonths(i) == 1)
        {
            if (usedStorage < (pricing->getGBStorage(i) * (long long)GB))
            {
                int currentAmountMonth = pricing->getAmountMonth(i);
                if (planNeeded == -1 || currentAmountMonth < amountPlanNeeded)
                {
                    planNeeded = i;
                    amountPlanNeeded = currentAmountMonth;
                }
            }
        }
    }

    return getReadablePlanFromId(pricing->getProLevel(planNeeded));
}

QString Utilities::getReadableStringFromTs(MegaIntegerList *list)
{
    if (!list || !list->size())
    {
        return QString();
    }

    QString readableTimes;
    for (int it = qMax(0, list->size() - 3); it < list->size(); it++)//Display only the most recent 3 times
    {
        int64_t ts = list->get(it);
        QDateTime date = QDateTime::fromMSecsSinceEpoch(ts * 1000l);
        readableTimes.append(QLocale().toString(date.date(), QLocale::LongFormat));

        if (it != list->size() - 1)
        {
            readableTimes.append(QStringLiteral(", "));
        }
    }

    return readableTimes;
}

QString Utilities::getReadablePlanFromId(int identifier, bool shortPlan)
{
    switch (identifier)
    {
        case MegaAccountDetails::ACCOUNT_TYPE_FREE:
            return QCoreApplication::translate("Utilities", "Free");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_STARTER:
            return shortPlan
                       ? QCoreApplication::translate("Utilities", "Starter")
                       : QCoreApplication::translate("Utilities", "MEGA Starter");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_BASIC:
            return shortPlan
                       ? QCoreApplication::translate("Utilities", "Basic")
                       : QCoreApplication::translate("Utilities", "MEGA Basic");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_ESSENTIAL:
            return shortPlan
                       ? QCoreApplication::translate("Utilities", "Essential")
                       : QCoreApplication::translate("Utilities", "MEGA Essential");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_LITE:
            return QCoreApplication::translate("Utilities", "Pro Lite");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_PROI:
            return QCoreApplication::translate("Utilities", "Pro I");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_PROII:
            return QCoreApplication::translate("Utilities", "Pro II");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_PROIII:
            return QCoreApplication::translate("Utilities", "Pro III");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_BUSINESS:
            return QCoreApplication::translate("Utilities", "Business");
            break;
        case MegaAccountDetails::ACCOUNT_TYPE_PRO_FLEXI:
            return QCoreApplication::translate("Utilities", "Pro Flexi");
            break;
    }

    return QString::fromUtf8("Pro");
}

void Utilities::animateFadein(QWidget *object, int msecs)
{
    animateProperty(object, msecs, "opacity", 0.0, 1.0);
}

void Utilities::animateFadeout(QWidget *object, int msecs)
{
    animateProperty(object, msecs, "opacity", 1.0, 0.0);
}

void Utilities::animatePartialFadein(QWidget *object, int msecs)
{
    animateProperty(object, msecs, "opacity", 0.5, 1.0);
}

void Utilities::animatePartialFadeout(QWidget *object, int msecs)
{
    animateProperty(object, msecs, "opacity", 1.0, 0.5);
}

void Utilities::animateProperty(QWidget *object, int msecs, const char * property, QVariant startValue, QVariant endValue, QEasingCurve curve)
{
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect();
    object->setGraphicsEffect(effect);
    auto animation = new QPropertyAnimation(effect, property);
    animation->setDuration(msecs);
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->setEasingCurve(curve);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

// Returns remaining days until given Unix timestamp in seconds.
void Utilities::getDaysToTimestamp(int64_t secsTimestamps, int64_t &remainDays)
{
    // Reference timestamp shouldn't be 0
    if (!secsTimestamps)
    {
        return;
    }

    int64_t hours = 0;
    getDaysAndHoursToTimestamp(secsTimestamps, remainDays, hours);
}

// Returns remaining days / hours until given Unix timestamp in seconds.
// Note: remainingHours and remaininDays represent the same value.
// i.e. for 1 day & 3 hours remaining, remainingHours will be 27, not 3.
void Utilities::getDaysAndHoursToTimestamp(int64_t secsTimestamps, int64_t &remainDays, int64_t &remainHours)
{
    // Reference timestamp shouldn't be 0
    if (!secsTimestamps)
    {
        return;
    }

    // Get seconcs diff between now and secsTimestamps
    int64_t tDiff = secsTimestamps - QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / MSEC_IN_1_SEC;

    // Compute in hours, then in days
    remainHours = tDiff / SECS_IN_1_HOUR;
    remainDays  = remainHours / HOURS_IN_1_DAY;
}

QString Utilities::getNonDuplicatedNodeName(MegaNode *node, MegaNode *parentNode, const QString &currentName, bool unescapeName, const QStringList& itemsBeingRenamed)
{
    QString newName;
    QString nodeName;
    QString suffix;

    if(node->isFile())
    {
        QFileInfo fileInfo(currentName);

        auto nameSplitted = Utilities::getFilenameBasenameAndSuffix(fileInfo.fileName());
        if(nameSplitted != QPair<QString, QString>())
        {
            nodeName = nameSplitted.first;
            suffix = nameSplitted.second;
        }
        else
        {
            nodeName = fileInfo.fileName();
        }
    }
    else
    {
        nodeName = currentName;
    }

    if(unescapeName)
    {
        nodeName = QString::fromUtf8(MegaSyncApp->getMegaApi()->unescapeFsIncompatible(nodeName.toUtf8().constData()));
    }

    int counter(1);
    while(newName.isEmpty())
    {
        bool nameFound = false;
        QString suggestedName = nodeName + QString(QLatin1String("(%1)")).arg(QString::number(counter));
        if(node)
        {
            if(node->isFile() && !suffix.isEmpty())
            {
                suggestedName.append(suffix);
            }
        }

        if(!itemsBeingRenamed.contains(suggestedName, Qt::CaseInsensitive))
        {
            std::unique_ptr<MegaNodeList>nodes(MegaSyncApp->getMegaApi()->getChildren(parentNode));
            for(int index = 0; index < nodes->size(); ++index)
            {
                QString nodeName(QString::fromUtf8(nodes->get(index)->getName()));
                if(suggestedName.compare(nodeName, Qt::CaseInsensitive) == 0)
                {
                    nameFound = true;
                }
            }


            if(!nameFound)
            {
                newName = suggestedName;
            }
        }

        counter++;
    }

    return newName;
}

QString Utilities::getNonDuplicatedLocalName(const QFileInfo &currentFile, bool unescapeName, const QStringList& itemsBeingRenamed)
{
    QString repeatedName;
    QString suffix = currentFile.completeSuffix();

    QString fileName;
    if(unescapeName)
    {
        fileName = QString::fromUtf8(MegaSyncApp->getMegaApi()->unescapeFsIncompatible(currentFile.baseName().toUtf8().constData()));
    }
    else
    {
        fileName = currentFile.baseName();
    }

    QDirIterator filesIt(currentFile.path(), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDirIterator::NoIteratorFlags);

    int counter(0);
    bool notFound(true);
    do
    {
        notFound = true;
        counter++;

        QString suggestedName = fileName + QString(QLatin1String("(%1)")).arg(QString::number(counter));
        if(!suffix.isEmpty())
        {
            suggestedName += QString(QLatin1String(".%1")).arg(suffix);
        }

        if(!itemsBeingRenamed.contains(suggestedName))
        {
            while (filesIt.hasNext())
            {
                QString checkFileName;

                if(unescapeName)
                {
                    checkFileName = QString::fromUtf8(MegaSyncApp->getMegaApi()->unescapeFsIncompatible(filesIt.fileName().toUtf8().constData()));
                }
                else
                {
                    checkFileName = filesIt.fileName();
                }

                if (checkFileName.compare(suggestedName, Qt::CaseInsensitive) == 0)
                {
                    notFound = false;
                    break;
                }

                filesIt.next();
            }
        }

    } while(!notFound);

    repeatedName = currentFile.baseName() + QString(QLatin1String("(%1)")).arg(QString::number(counter));
    if(!suffix.isEmpty())
    {
        repeatedName += QString(QLatin1String(".%1")).arg(suffix);
    }

    return repeatedName;
}

QPair<QString, QString> Utilities::getFilenameBasenameAndSuffix(const QString& fileName)
{
    QMimeDatabase db;
    int length = fileName.length();
    QList <QPair <int, QMimeType> > list;
    for (int index = length; index > -1; index--)
    {
        QList<QMimeType> mimes = db.mimeTypesForFileName(fileName.section(QLatin1String(""), index, length));
        QMimeType mime = mimes.isEmpty() ? QMimeType() : mimes.last();
        if (mime.isValid() && (list.isEmpty() || list.last().second != mime))
        {
            list.push_back(qMakePair(index, mime));
        }
    }

    QPair<QString, QString> result;

    if (!list.isEmpty())
    {
#ifdef Q_OS_LINUX
        result = qMakePair<QString, QString>(fileName.left(list.last().first), fileName.mid(list.last().first));
#else
        result = qMakePair<QString, QString>(fileName.left(list.last().first - 1), fileName.mid(list.last().first - 1));
#endif
    }

    return result;
}

void Utilities::upgradeClicked()
{
    QString url = QString::fromUtf8("mega://#pro");
    int accountType = Preferences::instance()->accountType();
    if(accountType == Preferences::ACCOUNT_TYPE_STARTER
        || accountType == Preferences::ACCOUNT_TYPE_BASIC
        || accountType == Preferences::ACCOUNT_TYPE_ESSENTIAL)
    {
        url.append(QString::fromUtf8("?tab=exc"));
    }
    getPROurlWithParameters(url);
    openUrl(QUrl(url));
}

QString Utilities::getNodePath(MegaTransfer* transfer)
{
    if (transfer->getPath() != nullptr)
    {
        return QString::fromUtf8(transfer->getPath());
    }
    return QString::fromUtf8(transfer->getParentPath()) + QString::fromUtf8(transfer->getFileName());
}

bool Utilities::isBusinessAccount()
{
    int accountType = Preferences::instance()->accountType();
    return accountType == Preferences::ACCOUNT_TYPE_BUSINESS
            || accountType == Preferences::ACCOUNT_TYPE_PRO_FLEXI;
}

QFuture<bool> Utilities::openUrl(QUrl url)
{
    return QtConcurrent::run(QDesktopServices::openUrl, url);
}

void Utilities::openInMega(MegaHandle handle)
{
    auto api (MegaSyncApp->getMegaApi());
    if (api)
    {
        std::unique_ptr<MegaNode> node (api->getNodeByHandle(handle));
        if (node)
        {
            std::unique_ptr<char[]> h (node->getBase64Handle());
            if(h)
            {
                openUrl(QUrl(QLatin1String("mega://#fm/") + QString::fromLatin1(h.get())));
            }
        }
    }
}

void Utilities::openBackupCenter()
{
    openUrl(QUrl(Utilities::BACKUP_CENTER_URL));
}

QString Utilities::getCommonPath(const QString &path1, const QString &path2, bool cloudPaths)
{
    QString ret;
    QString firstPath;
    QString secondPath;

    if(path1 < path2)
    {
        firstPath = cloudPaths ? path1 : QDir::toNativeSeparators(path1);
        secondPath = cloudPaths ? path2 : QDir::toNativeSeparators(path2);
    }
    else
    {
        firstPath = cloudPaths ? path2 : QDir::toNativeSeparators(path2);
        secondPath = cloudPaths ? path1 : QDir::toNativeSeparators(path1);
    }

    QString separator = cloudPaths ? QLatin1String("/") : QString(QDir::separator());
    if(!firstPath.endsWith(separator))
    {
        firstPath.append(separator);
    }

    if(!secondPath.endsWith(separator))
    {
        secondPath.append(separator);
    }
    if (firstPath == secondPath)
    {
        return path1;
    }
    int index = 1;
    while (firstPath.mid(0, index) == secondPath.mid(0, index))
    {
        ret = firstPath.mid(0, index);
        index++;
    }

    //Just in case the folder path contains common characters but they are not the same
    //EXAMPLE and EXAMPLE_1 for example
    //If ret does not end with a separator it is because folders are not the same
    if(!ret.endsWith(separator))
    {
        auto splittedPath = ret.split(separator);
        if(!splittedPath.isEmpty())
        {
            ret.remove(ret.lastIndexOf(separator) + 1,splittedPath.last().length());
        }
    }

    if(ret.endsWith(separator))
    {
        ret = ret.remove(ret.length() - 1, 1);
    }

    return ret;
}

bool Utilities::isIncommingShare(MegaNode *node)
{
    if(node && MegaSyncApp->getMegaApi()->checkAccess(node, MegaShare::ACCESS_OWNER).getErrorCode() != MegaError::API_OK)
    {
        return true;
    }

    return false;
}

bool Utilities::dayHasChangedSince(qint64 msecs)
{
    QDate currentDate = QDateTime::currentDateTime().date();
    QDate lastExecutionDate = QDateTime::fromMSecsSinceEpoch(msecs).date();
    return lastExecutionDate.daysTo(currentDate) > 0;
}

bool Utilities::monthHasChangedSince(qint64 msecs)
{
    QDate currentDate = QDateTime::currentDateTime().date();
    QDate lastExecutionDate = QDateTime::fromMSecsSinceEpoch(msecs).date();
    return lastExecutionDate.month() < currentDate.month();
}

QString Utilities::getTranslatedError(const MegaError* error)
{
    return QCoreApplication::translate("MegaError", error->getErrorString());
}

bool Utilities::restoreNode(MegaNode* node,
                            MegaApi* megaApi,
                            bool async,
                            std::function<void(MegaRequest*, MegaError*)> finishFunc)
{
    if (!node)
    {
        return false;
    }

    auto newParent = std::unique_ptr<MegaNode>(megaApi->getNodeByHandle(node->getRestoreHandle()));

    if (!newParent)
    {
        return false;
    }

    auto moveAnswer = [finishFunc](MegaRequest* request, MegaError* e) {
        if (finishFunc)
        {
            finishFunc(request, e);
        }
    };

    if (async)
    {
        auto listener =
            RequestListenerManager::instance().registerAndGetSynchronousFinishListener(moveAnswer);

        megaApi->moveNode(node, newParent.get(), listener.get());
    }
    else
    {
        MegaApiSynchronizedRequest::runRequestLambdaWithResult(
            [moveAnswer](MegaNode* node, MegaNode* targetNode, MegaRequestListener* listener) {
                MegaSyncApp->getMegaApi()->moveNode(node, targetNode, listener);
            },
            megaApi,
            moveAnswer,
            node,
            newParent.get());
    }

    return true;
}

bool Utilities::restoreNode(const char* nodeName,
                            const char* nodeDirectoryPath,
                            MegaApi* megaApi,
                            std::function<void(MegaRequest*, MegaError*)> finishFunc)
{
    std::unique_ptr<MegaNode> directoryNode(megaApi->getNodeByPath(nodeDirectoryPath));
    if (directoryNode)
    {
        MegaNode* rubbishNodeToRestore(nullptr);
        std::unique_ptr<MegaNodeList> rubbishNodes(
            MegaSyncApp->getMegaApi()->getChildren(MegaSyncApp->getRubbishNode().get()));
        for (auto index = 0; index < rubbishNodes->size(); ++index)
        {
            auto rubbishNode(rubbishNodes->get(index));
            if (rubbishNode)
            {
                if (rubbishNode->getRestoreHandle() == directoryNode->getHandle() &&
                    strcmp(nodeName, rubbishNode->getName()) == 0)
                {
                    if (!rubbishNodeToRestore ||
                        rubbishNodeToRestore->getCreationTime() < rubbishNode->getCreationTime())
                    {
                        rubbishNodeToRestore = rubbishNode;
                    }
                }
            }
        }

        if (rubbishNodeToRestore)
        {
            return restoreNode(rubbishNodeToRestore, megaApi, false, finishFunc);
        }

        return false;
    }
    else
    {
        QFileInfo directoryInfo(QString::fromUtf8(nodeDirectoryPath));
        return restoreNode(directoryInfo.fileName().toStdString().c_str(),
                           directoryInfo.path().toStdString().c_str(),
                           megaApi,
                           finishFunc);
    }
}

long long Utilities::getSystemsAvailableMemory()
{
    long long availMemory = 0;
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    memset(&statex, 0, sizeof (statex));
    statex.dwLength = sizeof (statex);
    if (GlobalMemoryStatusEx(&statex))
    {
        availMemory = std::min(statex.ullAvailPhys, statex.ullTotalVirtual);
    }
    else
    {
        std::cerr << "Error getting RAM usage info" << std::endl;
    }
#else
    long long pages = sysconf(_SC_PHYS_PAGES);
    long long page_size = sysconf(_SC_PAGE_SIZE);
    availMemory = (pages * page_size);
#endif
    return availMemory;
}

void Utilities::sleepMilliseconds(unsigned int milliseconds)
{
#ifdef WIN32
    Sleep(milliseconds);
#else
    // usleep accepts unsigned int argument.
    //Init to max allowed value, will be changed only if no overflow
    unsigned int usecs(std::numeric_limits<unsigned int>::max());

    // Avoid overflow. If no overflow, use passed value.
    if (milliseconds < (usecs/MSEC_IN_1_SEC))
    {
        usecs = milliseconds * MSEC_IN_1_SEC;
    }

    // Call safely
    usleep(usecs);
#endif
}

int Utilities::partPer(long long part, long long total, uint ref)
{
    // Use maximum precision
    long double partd(part);
    long double totald(total);
    long double refd(ref);

    // We can safely cast because the result should reasonably fit in an int.
    return (static_cast<int>((partd * refd) / totald));
}

bool Utilities::isNodeNameValid(const QString& name)
{
    QString trimmedName (name.trimmed());
    return !trimmedName.isEmpty() && !trimmedName.contains(FORBIDDEN_CHARS_RX);
}

QString Utilities::getFileHash(const QString& filePath)
{
    QFile file(filePath);

    // Verify precondition: the file must exist and be readable
    if (!QFile::exists(filePath) || (!file.open(QIODevice::ReadOnly)))
    {
        // Opening failed
        return QString::fromLatin1("");
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray buffer;

    while (!file.atEnd())
    {
        buffer = file.read(FILE_READ_BUFFER_SIZE);
        hash.addData(buffer);
    }

    file.close();

    QByteArray resultHash = hash.result();
    QString hashString = QString::fromUtf8(resultHash.toHex());

    return hashString;
}

void MegaListenerFuncExecuter::setExecuteInAppThread(bool executeInAppThread)
{
    mExecuteInAppThread = executeInAppThread;
}

void MegaListenerFuncExecuter::onRequestFinish(MegaApi *api, MegaRequest *request, MegaError *e)
{
    if (mExecuteInAppThread)
    {
        MegaRequest *requestCopy = request->copy();
        MegaError *errorCopy = e->copy();
        QObject temporary;
        QObject::connect(&temporary, &QObject::destroyed, qApp, [this, api, requestCopy, errorCopy](){

            if (onRequestFinishCallback)
            {
                onRequestFinishCallback(api, requestCopy, errorCopy);
            }

            if (mAutoremove)
            {
                delete this;
            }

        }, Qt::QueuedConnection);
    }
    else
    {
        if (onRequestFinishCallback)
        {
            onRequestFinishCallback(api, request, e);
        }

        if (mAutoremove)
        {
            delete this;
        }
    }
}

WrappedNode::WrappedNode(TransferOrigin from, MegaNode *node, bool undelete)
    : mTransfersFrom(from), mNode(node), mUndelete(undelete)
{
    qRegisterMetaType<QQueue<WrappedNode*>>("QQueue<WrappedNode*>");
}

TimeInterval::TimeInterval(long long secs, bool secondPrecision)
    : useSecondPrecision(secondPrecision)
{
    days = static_cast<int>(secs / SECS_IN_1_DAY);
    hours = static_cast<int>(secs / SECS_IN_1_HOUR) % HOURS_IN_1_DAY;
    minutes = static_cast<int>((secs / SECS_IN_1_MIN) % SECS_IN_1_MIN);
    seconds = static_cast<int>(secs % SECS_IN_1_MIN);
}

TimeInterval& TimeInterval::operator=(const TimeInterval& other)
{
    days = other.days;
    hours = other.hours;
    minutes = other.minutes;
    seconds = other.seconds;
    useSecondPrecision = other.useSecondPrecision;
    return *this;
}
