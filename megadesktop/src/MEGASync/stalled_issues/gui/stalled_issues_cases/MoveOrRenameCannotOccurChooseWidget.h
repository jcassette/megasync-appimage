#ifndef MOVEORRENAMECANNOTOCCURCHOOSEWIDGET_H
#define MOVEORRENAMECANNOTOCCURCHOOSEWIDGET_H

#include "MoveOrRenameCannotOccurIssue.h"
#include "StalledIssueChooseWidget.h"

class MoveOrRenameCannotOccurChooseWidget : public StalledIssueChooseWidget
{
    Q_OBJECT

public:
    explicit MoveOrRenameCannotOccurChooseWidget(QWidget *parent = nullptr);
    virtual ~MoveOrRenameCannotOccurChooseWidget() = default;

    void updateUi(std::shared_ptr<const MoveOrRenameCannotOccurIssue> issue);

protected:
    QString solvedString() const override { return QString(); }
    MoveOrRenameIssueChosenSide mChosenSide;
    std::shared_ptr<const MoveOrRenameCannotOccurIssue> mData;
};

class LocalMoveOrRenameCannotOccurChooseWidget : public MoveOrRenameCannotOccurChooseWidget
{
    Q_OBJECT

public:
    explicit LocalMoveOrRenameCannotOccurChooseWidget(QWidget *parent = nullptr);
    ~LocalMoveOrRenameCannotOccurChooseWidget() = default;

    void updateUi(std::shared_ptr<const MoveOrRenameCannotOccurIssue> issue);
};

class RemoteMoveOrRenameCannotOccurChooseWidget : public MoveOrRenameCannotOccurChooseWidget
{
    Q_OBJECT

public:
    explicit RemoteMoveOrRenameCannotOccurChooseWidget(QWidget *parent = nullptr);
    ~RemoteMoveOrRenameCannotOccurChooseWidget() = default;

    void updateUi(std::shared_ptr<const MoveOrRenameCannotOccurIssue> issue);
};

#endif // MOVEORRENAMECANNOTOCCURCHOOSEWIDGET_H
