#ifndef STALLEDISSUEDELEGATE_H
#define STALLEDISSUEDELEGATE_H

#include "StalledIssuesProxyModel.h"
#include "StalledIssuesModel.h"
#include "StalledIssuesDelegateWidgetsCache.h"

#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QPointer>

class StalledIssueBaseDelegateWidget;
class StalledIssuesView;

class StalledIssueDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    StalledIssueDelegate(StalledIssuesProxyModel* proxyModel,  StalledIssuesView* view);
    ~StalledIssueDelegate() = default;
    QSize sizeHint(const QStyleOptionViewItem&option, const QModelIndex&index) const override;
    void resetCache();

    void updateSizeHint();

    void expandIssue(const QModelIndex& sourceIndex);

public slots:
    void updateView();

signals:
    bool goToIssue(StalledIssueFilterCriterion filter, const QModelIndex& sourceIndex);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void destroyEditor(QWidget *, const QModelIndex &) const override;


protected slots:
    void onHoverEnter(const QModelIndex& index);
    void onHoverLeave(const QModelIndex&);
    void updateVisibleIndexesSizeHint(int updateDelay, bool forceUpdate);

private:
    QColor getRowColor(const QModelIndex& index) const;
    QModelIndex getEditorCurrentIndex() const;
    QModelIndex getRelativeIndex(const QModelIndex &index) const;
    QModelIndex getHeaderIndex(const QModelIndex& index) const;

    StalledIssueBaseDelegateWidget *getStalledIssueItemWidget(const QModelIndex &proxyIndex, const StalledIssueVariant &data, const QSize& size = QSize()) const;

    StalledIssuesView* mView;
    StalledIssuesProxyModel* mProxyModel;
    StalledIssuesModel* mSourceModel;
    StalledIssuesDelegateWidgetsCache mCacheManager;

    mutable QModelIndex mLastHoverIndex;

    mutable QPointer<StalledIssueBaseDelegateWidget> mEditor;

    QTimer mUpdateSizeHintTimerFromResize;
    QTimer mUpdateSizeHintTimer;
    QList<int> mVisibleIndexesRange;

    mutable int mSizeHintRequested = 0;
    mutable bool mFreshStart = true;
    mutable QMap<int, QPair<int, QSize>> mAverageHeaderHeight;
};

#endif // STALLEDISSUEDELEGATE_H
