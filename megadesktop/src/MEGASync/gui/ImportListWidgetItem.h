#ifndef IMPORTLISTWIDGETITEM_H
#define IMPORTLISTWIDGETITEM_H

#include <QListWidgetItem>
#include "megaapi.h"

namespace Ui {
class ImportListWidgetItem;
}

class ImportListWidgetItem : public QWidget
{
    Q_OBJECT

public:
    enum linkstatus {LOADING, CORRECT, WARNING, FAILED};

    explicit ImportListWidgetItem(QString link, int id, QWidget *parent = 0);
    ~ImportListWidgetItem();

    void setData(QString fileName, linkstatus status, long long size = 0, bool isFolder = false);
    void updateGui();
    bool isSelected();
    QString getLink();

private slots:
    void on_cSelected_stateChanged(int state);

signals:
    void stateChanged(int id, int state);

private:
    Ui::ImportListWidgetItem *ui;
    QString fileName;
    bool isFolder;
    linkstatus status;
    long long fileSize;
    QString link;
    int id;
};

#endif // IMPORTLISTWIDGETITEM_H
