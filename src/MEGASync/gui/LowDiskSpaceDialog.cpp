#include "LowDiskSpaceDialog.h"
#include "ui_LowDiskSpaceDialog.h"

#include "BlurredShadowEffect.h"
#include "Utilities.h"

#include <QFileIconProvider>

LowDiskSpaceDialog::LowDiskSpaceDialog(long neededSize,
    long long freeSize,
    long long driveSize,
    const DriveDisplayData& driveDisplayData,
    QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::LowDiskSpaceDialog)
{
    ui->setupUi(this);
    setupShadowEffect();

    auto message = tr("There is not enough space on %1. You need an additional %2 to download these files.");
    ui->lExplanation->setText(message.arg(driveDisplayData.name, toString(neededSize-freeSize)));

    ui->lDiskName->setText(driveDisplayData.name);
    ui->lFreeSpace->setText(tr("Free space: %1").arg(toString(freeSize)));
    ui->lTotalSize->setText(tr("Total size: %1").arg(toString(driveSize)));

    QIcon driveIcon(driveDisplayData.icon);
    ui->lDriveIcon->setPixmap(driveIcon.pixmap(64, 64));

    connect(ui->bTryAgain, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->bCancel, &QPushButton::clicked, this, &QDialog::reject);
}

LowDiskSpaceDialog::~LowDiskSpaceDialog()
{
    delete ui;
}

QString LowDiskSpaceDialog::toString(long long bytes)
{
    return Utilities::getSizeString((bytes > 0) ? bytes : 0);
}

void LowDiskSpaceDialog::setupShadowEffect()
{
#ifndef _WIN32
    ui->bTryAgain->setGraphicsEffect(CreateBlurredShadowEffect(QColor(54, 122, 246, 64), 1.0));
    ui->bCancel->setGraphicsEffect(CreateBlurredShadowEffect(1.0));
#endif
}

