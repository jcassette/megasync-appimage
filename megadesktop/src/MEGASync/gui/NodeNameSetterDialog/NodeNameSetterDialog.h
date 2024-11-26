#ifndef NODENAMESETTERDIALOG_H
#define NODENAMESETTERDIALOG_H

#include "QTMegaRequestListener.h"
#include <megaapi.h>

#include <QDialog>
#include <QTimer>

#include <memory>

namespace Ui {
class NodeNameSetterDialog;
}

class NodeNameSetterDialog : public QDialog, public mega::MegaRequestListener
{
    Q_OBJECT

public:
    NodeNameSetterDialog(QWidget* parent);
    virtual ~NodeNameSetterDialog();

    void init();
    QString getName() const;
    QString getOriginalName() const;

protected:
    virtual void onDialogAccepted() = 0;
    virtual void onRequestFinish(mega::MegaApi*, mega::MegaRequest*, mega::MegaError* ){}
    virtual QString dialogText() = 0;
    virtual void title() = 0;
    virtual QString lineEditText(){return QString();}

    struct LineEditSelection
    {
        int start;
        int length;

        LineEditSelection():start(0), length(0){}
    };
    virtual LineEditSelection lineEditSelection(){return LineEditSelection();}

    void showError(const QString& errorText);
    bool checkAlreadyExistingNode(const QString& nodeName, std::shared_ptr<mega::MegaNode> parentNode);
    void showAlreadyExistingNodeError(bool isFile);

    void changeEvent(QEvent* event);

    Ui::NodeNameSetterDialog* mUi;
    std::unique_ptr<mega::QTMegaRequestListener> mDelegateListener;

private:
    QTimer mNewFolderErrorTimer;
    QString mOriginalName;

private slots:
    void dialogAccepted();
    void newFolderErrorTimedOut();
};

#endif // NODENAMESETTERDIALOG_H
