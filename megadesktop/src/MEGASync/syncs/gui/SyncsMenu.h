#ifndef SYNCSMENU_H
#define SYNCSMENU_H

#include "MenuItemAction.h"
#include "SyncSettings.h"

#include "megaapi.h"

#include <QObject>
#include <QMenu>
#include <QPointer>

#include <memory>

namespace UserAttributes
{
class DeviceName;
class MyBackupsHandle;
}

class SyncsMenu : public QWidget
{
    Q_OBJECT

public:
    explicit SyncsMenu(mega::MegaSync::SyncType type, QObject* parent);

    static SyncsMenu* newSyncsMenu(mega::MegaSync::SyncType type, bool isEnabled, QWidget* parent);
    QPointer<MenuItemAction> getAction();
    void callMenu(const QPoint& p);
    void setEnabled(bool state);

signals:
    void addSync(mega::MegaSync::SyncType type);

protected:
    explicit SyncsMenu(mega::MegaSync::SyncType type,
                       int itemIndent,
                       const QIcon& iconMenu,
                       QWidget* parent);
    bool eventFilter(QObject* obj, QEvent* e) override;
    virtual QString createSyncTooltipText(const std::shared_ptr<SyncSettings>& syncSetting) const;
    virtual void refresh();

    QPointer<QMenu> getMenu();

    virtual QString getMenuActionText() const = 0;
    virtual QString getAddActionText() const = 0;

    QPointer<QMenu> mMenu;

private slots:
    void onAddSync();

private:
    void highLightMenuEntry(QAction *action);

    QPointer<MenuItemAction> mAddAction;
    QPointer<MenuItemAction> mMenuAction;
    QPointer<MenuItemAction> mLastHovered;
    mega::MegaSync::SyncType mType;
    int mItemIndent;
    QIcon mMenuIcon;

};

class TwoWaySyncsMenu : public SyncsMenu
{
    Q_OBJECT

public:
    explicit TwoWaySyncsMenu(QWidget* parent);

protected:
    QString getMenuActionText() const override;
    QString getAddActionText() const override;

private:
    QString createSyncTooltipText(const std::shared_ptr<SyncSettings>& syncSetting) const override;

    static const int mTwoWaySyncItemIndent = 0;
};

class BackupSyncsMenu : public SyncsMenu
{
    Q_OBJECT

public:
    explicit BackupSyncsMenu(QWidget* parent);

protected:
    QString getMenuActionText() const override;
    QString getAddActionText() const override;

private slots:
    void onDeviceNameSet(QString name);

private:
    QString createSyncTooltipText(const std::shared_ptr<SyncSettings>& syncSetting) const override;
    void refresh() override;

    static const int mBackupItemIndent = 1;
    QPointer<MenuItemAction> mDevNameAction;
    std::shared_ptr<UserAttributes::DeviceName> mDeviceNameRequest;
    std::shared_ptr<UserAttributes::MyBackupsHandle> mMyBackupsHandleRequest;
};


#endif // SYNCSMENU_H
