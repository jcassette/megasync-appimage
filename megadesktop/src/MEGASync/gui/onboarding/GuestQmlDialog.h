#ifndef GUESTQMLDIALOG_H
#define GUESTQMLDIALOG_H

#include "QmlDialog.h"

class GuestQmlDialog : public QmlDialog
{
    Q_OBJECT

public:
    explicit GuestQmlDialog(QWindow* parent = nullptr);
    bool isHiddenForLongTime() const;

signals:
    void guestActiveChanged(bool active);
    void hideRequested();

public slots:
    void realocate();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    qint64 mLastHideTime = 0;
};

#endif // GUESTQMLDIALOG_H
