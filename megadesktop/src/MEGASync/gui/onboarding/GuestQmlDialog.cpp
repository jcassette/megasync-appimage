#include "GuestQmlDialog.h"

#include "Platform.h"

GuestQmlDialog::GuestQmlDialog(QWindow *parent)
    : QmlDialog(parent)
{
    setFlags(flags() | Qt::FramelessWindowHint);

    QObject::connect(this, &GuestQmlDialog::activeChanged, [=]() {
        emit guestActiveChanged(this->isActive());
    });
}

bool GuestQmlDialog::isHiddenForLongTime() const
{
    return !isVisible() && QDateTime::currentMSecsSinceEpoch() - mLastHideTime > 500;
}

void GuestQmlDialog::realocate()
{
    int posx, posy;
    Platform::getInstance()->calculateInfoDialogCoordinates(geometry(), &posx, &posy);
    setX(posx);
    setY(posy);
}

void GuestQmlDialog::showEvent(QShowEvent *event)
{
    realocate();
    QmlDialog::showEvent(event);

    emit initializePageFocus();
}

void GuestQmlDialog::hideEvent(QHideEvent *event)
{
    mLastHideTime = QDateTime::currentMSecsSinceEpoch();
    QmlDialog::hideEvent(event);

    emit hideRequested();
}
