#ifndef QMEGAMESSAGEBOX_H
#define QMEGAMESSAGEBOX_H

#include <QMessageBox>
#include <QMap>
#include <QCheckBox>

class QMegaMessageBox : public QMessageBox
{
public:
    explicit QMegaMessageBox(QWidget* parent):
        QMessageBox(parent)
    {}

    static QString warningTitle();
    static QString errorTitle();

    struct MessageBoxInfo
    {
        std::function<void(QPointer<QMessageBox>)> finishFunc;
        QWidget* parent;
        QString title;
        QString text;
        QString informativeText;
        StandardButtons buttons;
        StandardButton defaultButton;
        QCheckBox* checkBox;
        QMap<StandardButton, QString> buttonsText;
        Qt::TextFormat textFormat;
        QPixmap iconPixmap;
        bool enqueue;
        bool ignoreCloseAll;
        QString checkboxText;

        MessageBoxInfo()
            : finishFunc(nullptr),
              parent(nullptr),
              buttons(Ok),
              defaultButton(NoButton),
              checkBox(nullptr),
              textFormat(Qt::PlainText),
              enqueue(false),
              ignoreCloseAll(false),
              checkboxText(QString())
        {}
    };

    static void information(const MessageBoxInfo& info);

    static void warning(const MessageBoxInfo& info);

    static void question(const MessageBoxInfo& info);

    static void critical(const MessageBoxInfo& info);

protected:
    bool event(QEvent *event) override;

private:
    static void showNewMessageBox(Icon icon, const MessageBoxInfo& info);
};

#endif // QMEGAMESSAGEBOX_H
