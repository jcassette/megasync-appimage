#include "NodeNameSetterDialog.h"
#include "ui_NodeNameSetterDialog.h"

#include <Utilities.h>
#include <MegaApplication.h>
#include <CommonMessages.h>
#include <mega/types.h>

#include <memory>

NodeNameSetterDialog::NodeNameSetterDialog(QWidget *parent)
    : QDialog(parent),
      mUi(new Ui::NodeNameSetterDialog()),
      mDelegateListener(std::make_unique<mega::QTMegaRequestListener>(MegaSyncApp->getMegaApi(), this))
{
}

NodeNameSetterDialog::~NodeNameSetterDialog()
{
    delete mUi;
}

void NodeNameSetterDialog::init()
{
    // Initialize the mNewFolder input Dialog
    mUi->setupUi(this);

    title();

    // The dialog doesn't get resized on error
    mUi->textLabel->setMinimumSize(mUi->errorLabel->sizeHint());

    connect(mUi->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    QPushButton *okButton = mUi->buttonBox->button(QDialogButtonBox::Ok);
    //only enabled when there's input, guards against empty folder name
    okButton->setEnabled(false);

    mOriginalName = lineEditText();
    mUi->lineEdit->setText(mOriginalName);
    mUi->lineEdit->setSelection(lineEditSelection().start, lineEditSelection().length);

    connect(mUi->lineEdit, &QLineEdit::textChanged, this, [this, okButton]()
    {
        bool hasText = !mUi->lineEdit->text().trimmed().isEmpty();
        okButton->setEnabled(hasText);
    });

    mUi->textLabel->setText(dialogText());

    mNewFolderErrorTimer.setSingleShot(true);
    connect(&mNewFolderErrorTimer, &QTimer::timeout, this, &NodeNameSetterDialog::newFolderErrorTimedOut);
    connect(mUi->buttonBox, &QDialogButtonBox::accepted, this, &NodeNameSetterDialog::dialogAccepted);
    connect(mUi->buttonBox, &QDialogButtonBox::rejected, this, [this]
    {
        reject();
    });

    mUi->errorLabel->hide();
    mUi->textLabel->show();
    mUi->lineEdit->setFocus();
}

QString NodeNameSetterDialog::getName() const
{
    return mUi->lineEdit->text().trimmed();
}

QString NodeNameSetterDialog::getOriginalName() const
{
    return mOriginalName;
}

void NodeNameSetterDialog::showError(const QString &errorText)
{
    mUi->errorLabel->setText(errorText);

    mUi->textLabel->hide();
    mUi->errorLabel->show();
    Utilities::animateFadein(mUi->errorLabel);
    mNewFolderErrorTimer.start(Utilities::ERROR_DISPLAY_TIME_MS); //(re)start timer
    mUi->lineEdit->setFocus();
}

bool NodeNameSetterDialog::checkAlreadyExistingNode(const QString& nodeName, std::shared_ptr<mega::MegaNode> parentNode)
{
    std::unique_ptr<mega::MegaNodeList>nodes(MegaSyncApp->getMegaApi()->getChildren(parentNode.get()));
    for(int index = 0; index < nodes->size(); ++index)
    {
        QString remoteNodeName(QString::fromUtf8(nodes->get(index)->getName()));
        if(nodeName.compare(remoteNodeName, Qt::CaseInsensitive) == 0)
        {
            showAlreadyExistingNodeError(nodes->get(index)->isFile());
            return true;
        }
    }

    return false;
}

void NodeNameSetterDialog::showAlreadyExistingNodeError(bool isFile)
{
    isFile ? showError(tr("A file with this name already exists in this location.\nEnter a different name."))
           : showError(tr("A folder with this name already exists in this location.\nEnter a different name"));
}

void NodeNameSetterDialog::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
         mUi->textLabel->setText(dialogText());
    }

    QDialog::changeEvent(event);
}

void NodeNameSetterDialog::dialogAccepted()
{
    //TODO add it when we have an answer from content team
    //auto stillExists = MegaSyncApp->getMegaApi()->getNodeByHandle(mParentNode->getHandle());

    //        if(!stillExists || MegaSyncApp->getMegaApi()->isInRubbish(mParentNode.get()))
    //        {
    //            showError(tr("Error\nParent folder removed."));
    //        }
    //        else
    {
        if(Utilities::isNodeNameValid(mUi->lineEdit->text()))
        {
            //dialog accepted, execute New Folder operation
            onDialogAccepted();
        }
        else
        {
            showError(CommonMessages::errorInvalidChars());
        }
    }
}

void NodeNameSetterDialog::newFolderErrorTimedOut()
{
    Utilities::animateFadeout(mUi->errorLabel);
    // after animation is finished, hide the error label and show the original text
    // 700 magic number is how long Utilities::  takes
    QTimer::singleShot(700, this, [this]()
    {
        mUi->errorLabel->hide();
        mUi->textLabel->show();
    });
}
