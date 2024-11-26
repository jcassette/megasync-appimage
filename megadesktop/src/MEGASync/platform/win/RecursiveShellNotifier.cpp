#include "RecursiveShellNotifier.h"

#include <QDir>

extern bool WindowsPlatform_exiting;

RecursiveShellNotifier::RecursiveShellNotifier(std::shared_ptr<AbstractShellNotifier> baseNotifier)
    : ShellNotifierDecorator(baseNotifier)
{
}

void RecursiveShellNotifier::notify(const QString& path)
{
    mBaseNotifier->notify(path);

    QStringList foldersToNotify;
    findFoldersRecursively(path, foldersToNotify);

    for (const auto& folder : qAsConst(foldersToNotify))
    {
        mBaseNotifier->notify(folder);
        if (WindowsPlatform_exiting) return;
    }

    emit shellNotificationProcessed();
}

void RecursiveShellNotifier::findFoldersRecursively(const QString &path, QStringList &folders)
{
    QDir dir(path);
    QFileInfoList children = dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const auto& child : qAsConst(children))
    {
        if (WindowsPlatform_exiting) return;
        QString childPath = QDir::toNativeSeparators(child.absoluteFilePath());
        folders.push_back(childPath);
        findFoldersRecursively(childPath, folders);
    }
}
