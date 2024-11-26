#include "StalledIssueHeader.h"

#include "StalledIssuesModel.h"
#include "StalledIssuesCaseHeaders.h"
#include "IgnoredStalledIssue.h"

#include <MegaApplication.h>

#include <QDialogButtonBox>
#include "Utilities.h"
#include <DialogOpener.h>
#include <StalledIssuesDialog.h>
#include <QMegaMessageBox.h>

#include <QFile>
#include <QMouseEvent>
#include <QCheckBox>

const int StalledIssueHeader::ARROW_INDENT = 6 + 16; //Left margin + arrow;
const int StalledIssueHeader::ICON_INDENT = 8 + 48; // fileIcon + spacer;
const int StalledIssueHeader::BODY_INDENT = StalledIssueHeader::ARROW_INDENT + StalledIssueHeader::ICON_INDENT; // full indent;
const int StalledIssueHeader::GROUPBOX_INDENT = BODY_INDENT - 9;// Following the InVision mockups
const int StalledIssueHeader::GROUPBOX_CONTENTS_INDENT = 9;// Following the InVision mockups
const int StalledIssueHeader::HEIGHT = 60;

const char* MULTIACTION_ICON = "MULTIACTION_ICON";
const char* FILENAME_PROPERTY = "FILENAME_PROPERTY";
const char* MULTIPLE_ACTIONS_PROPERTY = "ACTIONS_PROPERTY";
const char* ISSUE_STATE = "STATE";

StalledIssueHeader::StalledIssueHeader(QWidget *parent) :
    StalledIssueBaseDelegateWidget(parent),
    ui(new Ui::StalledIssueHeader),
    mIsExpandable(true)
{
    ui->setupUi(this);
    connect(ui->actionWaitingSpinner, &WaitingSpinnerWidget::needsUpdate, this, &StalledIssueHeader::needsUpdate);
    connect(ui->multipleActionButton, &QPushButton::clicked, this, &StalledIssueHeader::onMultipleActionClicked);
    ui->fileNameTitle->setTextFormat(Qt::TextFormat::AutoText);
    ui->errorDescriptionText->setTextFormat(Qt::TextFormat::AutoText);
    QSizePolicy sp_retain = ui->arrow->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    ui->arrow->setSizePolicy(sp_retain);
}

StalledIssueHeader::~StalledIssueHeader()
{
    delete ui;
}

void StalledIssueHeader::expand(bool state)
{
    if(mIsExpandable)
    {
        auto arrowIcon = Utilities::getCachedPixmap(
            state ? QLatin1String(":/images/node_selector/Icon-Small-Arrow-Down.png")
                  : QLatin1String(":/images/node_selector/Icon-Small-Arrow-Left.png"));
        ui->arrow->setPixmap(arrowIcon.pixmap(ui->arrow->size()));
    }
}

bool StalledIssueHeader::isExpandable() const
{
    return mIsExpandable;
}

void StalledIssueHeader::setIsExpandable(bool newIsExpandable)
{
    mIsExpandable = newIsExpandable;
    ui->arrow->setVisible(mIsExpandable);
}

bool StalledIssueHeader::adaptativeHeight()
{
    return false;
}

//For all issue with ignorable paths which are not special, hard or sym links
void StalledIssueHeader::onIgnoreFileActionClicked()
{
    propagateButtonClick();

    auto dialog = DialogOpener::findDialog<StalledIssuesDialog>();

    auto canBeIgnoredChecker = [](const std::shared_ptr<const StalledIssue> issue){
        return StalledIssue::convert<IgnoredStalledIssue>(issue) != nullptr;
    };

    QMegaMessageBox::MessageBoxInfo msgInfo;
    msgInfo.parent = dialog ? dialog->getDialog() : nullptr;
    msgInfo.title = MegaSyncApp->getMEGAString();
    msgInfo.textFormat = Qt::RichText;
    msgInfo.buttons = QMessageBox::Ok | QMessageBox::Cancel;
    QMap<QMessageBox::Button, QString> textsByButton;

    auto selection = dialog->getDialog()->getSelection(canBeIgnoredChecker);
    textsByButton.insert(QMessageBox::Ok, tr("Apply"));
    auto allSimilarIssues = MegaSyncApp->getStalledIssuesModel()->getIssues(canBeIgnoredChecker);
    if(allSimilarIssues.size() != selection.size())
    {
        auto checkBox = new QCheckBox(tr("Apply to all"));
        msgInfo.checkBox = checkBox;
    }
    msgInfo.buttonsText = textsByButton;
    msgInfo.text = tr("Are you sure you want to ignore this issue?");
    msgInfo.informativeText = tr("This action will ignore this issue and it will not be synced.");

    msgInfo.finishFunc = [selection](QMessageBox* msgBox)
    {
        if(msgBox->result() == QDialogButtonBox::Ok)
        {
            if(msgBox->checkBox() && msgBox->checkBox()->isChecked())
            {
                MegaSyncApp->getStalledIssuesModel()->ignoreAllSimilarIssues();
            }
            else
            {
            MegaSyncApp->getStalledIssuesModel()->ignoreItems(selection);
            }
        }
    };

    QMegaMessageBox::warning(msgInfo);

}

void StalledIssueHeader::showIgnoreFile()
{
    StalledIssueHeader::ActionInfo action(tr("Ignore"), ActionsId::Ignore);
    showAction(action);
}

void StalledIssueHeader::propagateButtonClick()
{
    QApplication::postEvent(this, new QMouseEvent(QEvent::MouseButtonPress, QPointF(), Qt::LeftButton, Qt::NoButton, Qt::KeyboardModifier::AltModifier));
    qApp->processEvents();
}

void StalledIssueHeader::showAction(const ActionInfo& action)
{
    showActions(QString(), QList<ActionInfo>() << action);
}

void StalledIssueHeader::showActions(const QString &actionButtonText, const QList<ActionInfo>& actions)
{
    ui->actionContainer->show();
    ui->multipleActionButton->setVisible(true);
    if(actions.size() == 1)
    {
        ui->multipleActionButton->setText(actions.first().actionText);
        if(!ui->multipleActionButton->icon().isNull())
        {
            ui->multipleActionButton->setProperty(MULTIACTION_ICON, QVariant::fromValue<QIcon>(ui->multipleActionButton->icon()));
            ui->multipleActionButton->setIcon(QIcon());
        }
    }
    else
    {
        ui->multipleActionButton->setText(actionButtonText);
        if(ui->multipleActionButton->icon().isNull())
        {
            ui->multipleActionButton->setIcon(ui->multipleActionButton->property(MULTIACTION_ICON).value<QIcon>());
        }
    }

    ui->multipleActionButton->setProperty(MULTIPLE_ACTIONS_PROPERTY, QVariant::fromValue<QList<ActionInfo>>(actions));
}

void StalledIssueHeader::hideAction()
{
    ui->multipleActionButton->setVisible(false);
}

void StalledIssueHeader::onMultipleActionClicked()
{
    propagateButtonClick();

    auto actions(ui->multipleActionButton->property(MULTIPLE_ACTIONS_PROPERTY).value<QList<ActionInfo>>());
    if(!actions.isEmpty())
    {
        if(actions.size() == 1)
        {
            if(actions.first().id == ActionsId::Ignore)
            {
                onIgnoreFileActionClicked();
            }
            else
            {
                mHeaderCase->onMultipleActionButtonOptionSelected(this, actions.first().id);
            }
        }
        else
        {
            QMenu *menu(new QMenu(ui->multipleActionButton));
            Platform::getInstance()->initMenu(menu, "MultipleActionStalledIssues");
            menu->setAttribute(Qt::WA_DeleteOnClose);

            foreach(auto action, actions)
            {
                // Show in system file explorer action
                auto actionItem (new MenuItemAction(action.actionText, QString()));
                auto id(action.id);
                connect(actionItem, &MenuItemAction::triggered, this, [this, id]()
                {
                    mHeaderCase->onMultipleActionButtonOptionSelected(this, id);
                });
                actionItem->setParent(menu);
                menu->addAction(actionItem);
            }

            auto pos(ui->actionContainer->mapToGlobal(ui->multipleActionButton->pos()));
            pos.setY(pos.y() + ui->multipleActionButton->height());
            menu->popup(pos);
        }
    }
}

void StalledIssueHeader::showMessage(const QString &message, const QPixmap& pixmap)
{
    ui->actionContainer->show();
    if(!message.isEmpty() || !pixmap.isNull())
    {
        ui->actionMessageContainer->setVisible(true);
    }

    ui->actionMessage->setText(message);

    if(!pixmap.isNull())
    {
        ui->actionMessageIcon->show();
        ui->actionMessageIcon->setPixmap(pixmap);
    }
    else
    {
        ui->actionMessageIcon->hide();
    }
}

void StalledIssueHeader::updateIssueState()
{
    auto type(getData().consultData()->getIsSolved());

    if(type == StalledIssue::SolveType::BEING_SOLVED)
    {
        ui->actionWaitingSpinner->start();
    }
    else
    {
        ui->actionWaitingSpinner->stop();
    }

    QIcon icon;
    QString message;

    switch(type)
    {
        case StalledIssue::SolveType::BEING_SOLVED:
        {
            ui->actionMessageContainer->setProperty(ISSUE_STATE, QLatin1String("being solved"));
            message = tr("Being solved");
            break;
        }
        case StalledIssue::SolveType::SOLVED:
        {
            ui->actionMessageContainer->setProperty(ISSUE_STATE, QLatin1String("solved"));
            if(getData().convert<IgnoredStalledIssue>())
            {
                icon = QIcon(QString::fromUtf8(":/images/StalledIssues/states/solved_state.png"));
                message = tr("Ignored");
            }
            else
            {
                if(getData().consultData()->wasAutoResolutionApplied())
                {
                    icon = QIcon(QString::fromUtf8(":/images/StalledIssues/states/auto_solved_state.png"));
                    message = tr("Auto-solved");
                }
                else
                {
                    icon = QIcon(QString::fromUtf8(":/images/StalledIssues/states/solved_state.png"));
                    message = tr("Solved");
                }
            }

            break;
        }
        case StalledIssue::SolveType::FAILED:
        {
            ui->actionMessageContainer->setProperty(ISSUE_STATE, QLatin1String("failed"));
            icon = QIcon(QString::fromUtf8(":/images/StalledIssues/states/failed_state.png"));
            message = tr("Failed");

            break;
        }
        case StalledIssue::SolveType::UNSOLVED:
        {
            ui->actionMessageContainer->setProperty(ISSUE_STATE, QString());
            if(getData().convert<IgnoredStalledIssue>())
            {
                showIgnoreFile();
            }
            break;
        }
        default:
        {
            break;
        }
    }

    showMessage(message, icon.pixmap(16, 16));
    ui->actionMessageContainer->setStyleSheet(ui->actionMessageContainer->styleSheet());
}

void StalledIssueHeader::setText(const QString &text, const QString& tooltip)
{
    if(text.isEmpty())
    {
        ui->fileNameTitle->hide();
    }
    else
    {
        ui->fileNameTitle->show();
        ui->fileNameTitle->setText(text);
        ui->fileNameTitle->setToolTip(tooltip);
    }
}

QString StalledIssueHeader::displayFileName(bool preferCloud)
{
    return getData().consultData()->getFileName(preferCloud);
}

void StalledIssueHeader::setTitleDescriptionText(const QString &text)
{
    if(text.isEmpty())
    {
        ui->errorDescription->hide();
    }
    else
    {
        ui->errorDescription->show();
        ui->errorDescriptionText->setText(text);
    }
}

void StalledIssueHeader::setData(StalledIssueHeaderCase * issueData)
{
    mHeaderCase = issueData;
}

void StalledIssueHeader::reset()
{
    StalledIssueBaseDelegateWidget::reset();
    mHeaderCase->deleteLater();
}

void StalledIssueHeader::refreshCaseTitles()
{
    if(mHeaderCase)
    {
        //Reset strings first
        setText(QString());
        setTitleDescriptionText(QString());

        mHeaderCase->refreshCaseTitles(this);
    }
}

void StalledIssueHeader::refreshCaseActions()
{
    if(mHeaderCase)
    {
        mHeaderCase->refreshCaseActions(this);
        updateHeaderSizes();
    }
}

void StalledIssueHeader::updateHeaderSizes()
{
    //Why two times? not sure, but works
    for(int times = 0; times < 2; ++times)
    {
        layout()->activate();
        updateGeometry();

        ui->actionContainer->layout()->activate();
        ui->actionContainer->updateGeometry();

        ui->titleContainer->layout()->activate();
        ui->titleContainer->updateGeometry();

        ui->errorContainer->layout()->activate();
        ui->errorContainer->updateGeometry();

        ui->errorDescription->layout()->activate();
        ui->errorDescription->updateGeometry();

        ui->errorTitle->layout()->activate();
        ui->errorTitle->updateGeometry();

        ui->errorTitleTextContainer->layout()->activate();
        ui->errorTitleTextContainer->updateGeometry();

        ui->fileNameTitle->updateGeometry();
        ui->errorDescriptionText->updateGeometry();
    }
}

QString StalledIssueHeader::fileName()
{
    return QString();
}

void StalledIssueHeader::refreshUi()
{
    auto errorTitleIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/ico_menu_full.png"));
    ui->errorTitleIcon->setPixmap(errorTitleIcon.pixmap(ui->errorTitleIcon->size()));

    QIcon fileTypeIcon;
    QFileInfo fileInfo;

    //Get full path -> it can be taken from the cloud data or the local data.
    if(getData().consultData()->consultLocalData())
    {
        fileInfo.setFile(getData().consultData()->consultLocalData()->getNativeFilePath());
    }
    else if(getData().consultData()->consultCloudData())
    {
        fileInfo.setFile(getData().consultData()->consultCloudData()->getNativeFilePath());
    }

    if(getData().consultData()->filesCount() > 0)
    {
        fileTypeIcon = Utilities::getCachedPixmap(Utilities::getExtensionPixmapName(
                                                      getData().consultData()->getFileName(false), QLatin1String(":/images/drag_")));
    }
    else
    {
        fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/folder_orange_default@2x.png"));
    }

    ui->fileTypeIcon->setPixmap(fileTypeIcon.pixmap(ui->fileTypeIcon->size()));

    resetSolvingWidgets();

    updateIssueState();

    //By default it is expandable
    if(getData().consultData())
    {
        setIsExpandable(getData().consultData()->isExpandable());
    }
}

void StalledIssueHeader::resetSolvingWidgets()
{
    ui->multipleActionButton->hide();
    ui->actionMessageContainer->hide();
}
