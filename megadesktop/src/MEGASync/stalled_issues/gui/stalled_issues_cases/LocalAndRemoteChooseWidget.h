#ifndef LOCALANDREMOTECHOOSEWIDGET_H
#define LOCALANDREMOTECHOOSEWIDGET_H

#include <StalledIssueChooseWidget.h>

class StalledIssueChooseWidget;

class LocalAndRemoteStalledIssueBaseChooseWidget : public StalledIssueChooseWidget
{
    Q_OBJECT

public:
    explicit LocalAndRemoteStalledIssueBaseChooseWidget(QWidget *parent = nullptr);
    virtual ~LocalAndRemoteStalledIssueBaseChooseWidget() = default;

    void updateUi(StalledIssueDataPtr data,
                  LocalOrRemoteUserMustChooseStalledIssue::ChosenSide side =
                      LocalOrRemoteUserMustChooseStalledIssue::ChosenSide::NONE);
    const StalledIssueDataPtr& data();

protected:
    StalledIssueDataPtr mData;
};

class LocalStalledIssueChooseWidget : public LocalAndRemoteStalledIssueBaseChooseWidget
{
    Q_OBJECT

public:
    explicit LocalStalledIssueChooseWidget(QWidget *parent = nullptr)
        : LocalAndRemoteStalledIssueBaseChooseWidget(parent)
    {}

    ~LocalStalledIssueChooseWidget() = default;

    QString solvedString() const override;
    void updateUi(LocalStalledIssueDataPtr localData,
                  LocalOrRemoteUserMustChooseStalledIssue::ChosenSide side =
                      LocalOrRemoteUserMustChooseStalledIssue::ChosenSide::NONE);

protected slots:
    void onRawInfoToggled() override;

private:
    void updateExtraInfo(LocalStalledIssueDataPtr data);
};

class CloudStalledIssueChooseWidget : public LocalAndRemoteStalledIssueBaseChooseWidget
{
    Q_OBJECT

public:
    explicit CloudStalledIssueChooseWidget(QWidget *parent = nullptr)
        : LocalAndRemoteStalledIssueBaseChooseWidget(parent)
    {}

    ~CloudStalledIssueChooseWidget() = default;

    QString solvedString() const override;
    void updateUi(CloudStalledIssueDataPtr cloudData,
                  LocalOrRemoteUserMustChooseStalledIssue::ChosenSide side =
                      LocalOrRemoteUserMustChooseStalledIssue::ChosenSide::NONE);

protected slots:
    void onRawInfoToggled() override;

private:
    void updateExtraInfo(CloudStalledIssueDataPtr data);
};



#endif // LOCALANDREMOTECHOOSEWIDGET_H
