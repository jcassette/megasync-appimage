#ifndef AVATARWIDGET_H
#define AVATARWIDGET_H

#include <QWidget>

#include <memory>

namespace UserAttributes{
   class Avatar;
}

class AvatarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AvatarWidget(QWidget* parent = 0);

    void setUserEmail(const char *userEmail = nullptr);
    void clearData();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

signals:
    void avatarUpdated();

protected:
    void paintEvent(QPaintEvent* event);
    void mousePressEvent(QMouseEvent* event);

private:
    std::shared_ptr<const UserAttributes::Avatar> mAvatarRequest;
    QMetaObject::Connection mAvatarConnection;
};

class AvatarPixmap
{
public:
    static QPixmap maskFromImagePath(const QString& pathToFile, int size);
    static QPixmap createFromLetter(const QString& letter, const QColor& primaryColor, const QColor& secondaryColor, int size);
};

#endif // AVATARWIDGET_H
