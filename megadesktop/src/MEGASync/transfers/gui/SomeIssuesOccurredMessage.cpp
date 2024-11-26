#include "SomeIssuesOccurredMessage.h"
#include "ui_SomeIssuesOccurredMessage.h"

#include <StalledIssuesDialog.h>
#include <DialogOpener.h>
#include <Platform.h>
#include <StalledIssuesModel.h>

SomeIssuesOccurredMessage::SomeIssuesOccurredMessage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SomeIssuesOccurredMessage)
{
    ui->setupUi(this);
}

SomeIssuesOccurredMessage::~SomeIssuesOccurredMessage()
{
    delete ui;
}

void SomeIssuesOccurredMessage::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(event);
}

void SomeIssuesOccurredMessage::on_viewIssuesButton_clicked()
{
    auto stalledIssuesDialog = DialogOpener::findDialog<StalledIssuesDialog>();
    if(stalledIssuesDialog)
    {
        DialogOpener::showGeometryRetainerDialog(stalledIssuesDialog->getDialog());
    }
    else
    {
        auto newStalledIssuesDialog = new StalledIssuesDialog();
        DialogOpener::showDialog<StalledIssuesDialog>(newStalledIssuesDialog);
    }
}
