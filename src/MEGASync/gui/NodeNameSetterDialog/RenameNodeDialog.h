#ifndef RENAMENODEDIALOG_H
#define RENAMENODEDIALOG_H

#include "NodeNameSetterDialog.h"

#include <megaapi.h>

#include <memory>

class RenameNodeDialog : public NodeNameSetterDialog
{
    Q_OBJECT

public:
    RenameNodeDialog(QWidget* parent);
    virtual ~RenameNodeDialog() = default;

protected:
    QString dialogText() override;
    NodeNameSetterDialog::LineEditSelection lineEditSelection() override;

    QString enterNewFileNameText() const;
    QString enterNewFolderNameText() const;
    void title() override;

    virtual bool isFile() = 0;
};

class RenameRemoteNodeDialog : public RenameNodeDialog
{
    Q_OBJECT

public:
    RenameRemoteNodeDialog(const QString& nodeName, QWidget* parent);
    RenameRemoteNodeDialog(std::unique_ptr<mega::MegaNode>, QWidget* parent);

    ~RenameRemoteNodeDialog() = default;

    static QString renamedFailedErrorString(mega::MegaError* error, bool isFile);

protected:
    void onDialogAccepted() override;
    void onRequestFinish(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError* e) override;
    bool isFile() override;
    QString lineEditText() override;

private:
    void showRenamedFailedError(mega::MegaError* error);

    std::unique_ptr<mega::MegaNode> mNodeToRename;
    QString mNodeName;
};

class RenameLocalNodeDialog : public RenameNodeDialog
{
    Q_OBJECT

public:
    RenameLocalNodeDialog(const QString& nodePath, QWidget* parent);
    ~RenameLocalNodeDialog() = default;

    static QString renamedFailedErrorString(bool isFile);

protected:
    void onDialogAccepted() override;
    bool isFile() override;
    QString lineEditText() override;

private:
    void showRenamedFailedError();

    QString mNodePath;
};

#endif // RENAMENODEDIALOG_H
