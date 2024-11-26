#include "StalledIssueDelegate.h"

#include "StalledIssueFilePath.h"
#include "StalledIssueBaseDelegateWidget.h"
#include "StalledIssueHeader.h"
#include "StalledIssuesView.h"
#include "MegaDelegateHoverManager.h"
#include "StalledIssue.h"
#include <DialogOpener.h>
#include <StalledIssuesDialog.h>

#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QElapsedTimer>
#include <QPainterPath>

#ifdef Q_OS_MACOS
const QColor HOVER_COLOR = QColor("#F7F7F7");
#else
const QColor HOVER_COLOR = QColor("#FAFAFA");
#endif
const QColor SELECTED_BORDER_COLOR = QColor("#E9E9E9");
const float LEFT_MARGIN = 4.0;
const float RIGHT_MARGIN = 6.0;
const float RECT_BORDERS_MARGIN = 20.0;
const float BOTTON_MARGIN = 6.0;
const float TOP_MARGIN = 2.0;
const float CORNER_RADIUS = 10.0;
const int PEN_WIDTH = 2;
const int UPDATE_SIZE_TIMER = 50;

const QSize DEFAULT_SIZE = QSize(100,60);

StalledIssueDelegate::StalledIssueDelegate(StalledIssuesProxyModel* proxyModel,  StalledIssuesView *view)
    :QStyledItemDelegate(view),
     mView(view),
     mProxyModel (proxyModel),
     mCacheManager(this),
     mEditor(nullptr)
{
    mSourceModel = qobject_cast<StalledIssuesModel*>(
                      mProxyModel->sourceModel());


    connect(mProxyModel, &StalledIssuesProxyModel::modelFiltered, this, [this](){
        mSizeHintRequested = mProxyModel->rowCount(QModelIndex());
        if(mSizeHintRequested == 0)
        {
            mSourceModel->unBlockUi();
        }
        else
        {
            mFreshStart = true;
            mAverageHeaderHeight.clear();
        }

        updateVisibleIndexesSizeHint(UPDATE_SIZE_TIMER, true);
    });

    mCacheManager.setProxyModel(mProxyModel);

    mUpdateSizeHintTimer.setSingleShot(true);
    connect(&mUpdateSizeHintTimer, &QTimer::timeout, this, [this](){
        if(mSizeHintRequested == 0)
        {
            mSizeHintRequested = mProxyModel->rowCount(QModelIndex());
            emit sizeHintChanged(QModelIndex());
        }
    });

    connect(mView, &StalledIssuesView::scrollStopped, this, [this](){
        updateVisibleIndexesSizeHint(UPDATE_SIZE_TIMER, false);
    });

    mUpdateSizeHintTimerFromResize.setSingleShot(true);
    connect(&mUpdateSizeHintTimerFromResize, &QTimer::timeout, this, [this](){
        updateVisibleIndexesSizeHint(UPDATE_SIZE_TIMER, true);
    });

    mView->installEventFilter(this);
}

void StalledIssueDelegate::updateVisibleIndexesSizeHint(int updateDelay, bool forceUpdate)
{
    if(forceUpdate)
    {
        mVisibleIndexesRange.clear();
    }

    auto firstIndex = mView->indexAt(QPoint(0,0));
    auto firstRow = firstIndex.parent().isValid() ? firstIndex.parent().row() : firstIndex.row();

    int delegateWidgetMiddleSize(StalledIssuesDelegateWidgetsCache::DELEGATEWIDGETS_CACHESIZE/2);

    bool sizeHintUpdateNeeded(false);

    for(int row = firstRow - delegateWidgetMiddleSize; row <= (firstRow + delegateWidgetMiddleSize); ++row)
    {
        if(row >= 0)
        {
            if(!mVisibleIndexesRange.contains(row))
            {
                auto index = mProxyModel->index(row, 0);
                if(index.isValid())
                {
                    if(mVisibleIndexesRange.size() == StalledIssuesDelegateWidgetsCache::DELEGATEWIDGETS_CACHESIZE)
                    {
                        mVisibleIndexesRange.removeFirst();
                    }

                    mVisibleIndexesRange.append(row);

                    StalledIssueVariant stalledIssueItem (qvariant_cast<StalledIssueVariant>(index.data(Qt::DisplayRole)));
                    if(stalledIssueItem.isValid())
                    {
                        stalledIssueItem.removeDelegateSize(StalledIssue::Header);
                        stalledIssueItem.removeDelegateSize(StalledIssue::Body);
                    }

                    sizeHintUpdateNeeded = true;
                }

            }
        }
    }

    if(sizeHintUpdateNeeded)
    {
        mUpdateSizeHintTimer.start(updateDelay);
    }
}

QSize StalledIssueDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if(mSizeHintRequested > 0)
    {
        mSizeHintRequested--;

        if(mSizeHintRequested == 0)
        {
            mSourceModel->unBlockUi();
        }
    }

    StalledIssueVariant stalledIssueItem (qvariant_cast<StalledIssueVariant>(index.data(Qt::DisplayRole)));
    
    if(stalledIssueItem.consultData())
    {
        QSize size;

        StalledIssue::Type sizeType = index.parent().isValid() ? StalledIssue::Body : StalledIssue::Header;
        size = stalledIssueItem.getDelegateSize(sizeType);

        if(!size.isValid() && mFreshStart)
        {
            auto parentRow(index.parent().isValid() ? index.parent().row() : index.row());
            if((mVisibleIndexesRange.isEmpty() && parentRow > StalledIssuesDelegateWidgetsCache::DELEGATEWIDGETS_CACHESIZE)
               || (!mVisibleIndexesRange.isEmpty() && !mVisibleIndexesRange.contains(parentRow)))
            {
                auto averageSizeInfo(mAverageHeaderHeight.value(stalledIssueItem.consultData()->getReason()));
                auto averageSize = averageSizeInfo.second;

                if(averageSize.isValid())
                {
                    size = QSize(averageSize.width(), averageSize.height()/averageSizeInfo.first);
                }
                else
                {
                    size = DEFAULT_SIZE;
                }

                stalledIssueItem.setDelegateSize(size, StalledIssue::Header);
            }
        }

        if(!size.isValid())
        {
            QSize proposedSize;
            auto parentIndex(index.parent());
            if(parentIndex.isValid())
            {
                StalledIssueVariant parentStalledIssueItem (qvariant_cast<StalledIssueVariant>(parentIndex.data(Qt::DisplayRole)));
                StalledIssueBaseDelegateWidget* parentW (getStalledIssueItemWidget(parentIndex, parentStalledIssueItem));
                if(parentW)
                {
                    proposedSize = parentW->size();
                }
            }
            else
            {
                auto dialog = DialogOpener::findDialog<StalledIssuesDialog>();
                proposedSize = dialog->getDialog()->size();
            }

            StalledIssueBaseDelegateWidget* w (getStalledIssueItemWidget(index, stalledIssueItem, proposedSize));
            if(w)
            {
                size = w->sizeHint();
                if(mFreshStart)
                {
                    if(mAverageHeaderHeight.contains(stalledIssueItem.consultData()->getReason()))
                    {
                        auto& info = mAverageHeaderHeight[stalledIssueItem.consultData()->getReason()];
                        info.first++;
                        info.second = QSize(info.second.width(), info.second.height() + size.height());
                    }
                    else
                    {
                        mAverageHeaderHeight.insert(stalledIssueItem.consultData()->getReason(), qMakePair(1, size));
                    }
                }
            }
        }

        if(size.isValid())
        {
            return size;
        }
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

void StalledIssueDelegate::resetCache()
{
    mCacheManager.reset();
}

void StalledIssueDelegate::updateView()
{
    mView->viewport()->update();
}

void StalledIssueDelegate::updateSizeHint()
{
    mUpdateSizeHintTimer.start(UPDATE_SIZE_TIMER);
}

void StalledIssueDelegate::expandIssue(const QModelIndex& sourceIndex)
{
    mView->expand(sourceIndex);
}

void StalledIssueDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    mFreshStart = false;

    auto rowCount (index.model()->rowCount());
    auto row (index.row());

    if (index.isValid() && row < rowCount)
    {
        auto stalledIssueItem (qvariant_cast<StalledIssueVariant>(index.data(Qt::DisplayRole)));

        bool isExpanded(mView->isExpanded(index) && stalledIssueItem.consultData()->isExpandable());

        auto pos (option.rect.topLeft());
        QRect geometry(option.rect);
        QRectF realGeometry(option.rect);

#ifdef __APPLE__
        auto width = mView->size().width();
        width -= mView->contentsMargins().left();
        width -= mView->contentsMargins().right();
        if(mView->verticalScrollBar() && mView->verticalScrollBar()->isVisible())
        {
            width -= mView->verticalScrollBar()->width();
        }
        geometry.setWidth(std::min(width, option.rect.width()));
#endif

        auto rowColor = Qt::white;
        auto backgroundRect = isExpanded ? geometry : option.rect.adjusted(0,0,0,-1);

        QStyle::State state = option.state;

        auto headerIndex(getHeaderIndex(index));
        if(!(state & QStyle::State_MouseOver) && mLastHoverIndex == headerIndex)
        {
            state |= QStyle::State_MouseOver;
        }

        if((state & QStyle::State_MouseOver) && mLastHoverIndex != headerIndex)
        {
            state &= ~QStyle::State_MouseOver;
        }

        if(index.parent().isValid())
        {
            if(mView->selectionModel()->isSelected(index.parent()))
            {
                state |= QStyle::State_Selected;
            }
        }

        QColor fillColor;
        QPen pen;
        QPainterPath path;

        if(state & (QStyle::State_MouseOver | QStyle::State_Selected))
        {
            path.setFillRule( Qt::WindingFill );
            path.addRoundedRect(QRectF(realGeometry.x() + LEFT_MARGIN,
                                       realGeometry.y() + TOP_MARGIN,
                                       realGeometry.width() - RIGHT_MARGIN,
                                       realGeometry.height() - BOTTON_MARGIN), CORNER_RADIUS, CORNER_RADIUS);

            if(index.parent().isValid())
            {
                path.addRect(QRectF(realGeometry.x() + LEFT_MARGIN,
                                    realGeometry.y(),
                                    realGeometry.width() - RIGHT_MARGIN,
                                    RECT_BORDERS_MARGIN)); // Bottom corners not rounded
            }
            else if(isExpanded)
            {
                path.addRect(QRectF(realGeometry.x() + LEFT_MARGIN,
                                    realGeometry.y() + RECT_BORDERS_MARGIN,
                                    realGeometry.width() - RIGHT_MARGIN,
                                    realGeometry.height() - RECT_BORDERS_MARGIN)); // Bottom corners not rounded
            }

            if(state & QStyle::State_MouseOver && state & QStyle::State_Selected)
            {
                pen.setColor(SELECTED_BORDER_COLOR);
                pen.setWidth(PEN_WIDTH);
                fillColor = HOVER_COLOR;
            }
            else
            {
                if(state & QStyle::State_MouseOver)
                {
                    pen.setColor(HOVER_COLOR);
                    fillColor = HOVER_COLOR;
                }
                else if (state & QStyle::State_Selected)
                {
                    pen.setColor(SELECTED_BORDER_COLOR);
                    pen.setWidth(PEN_WIDTH);
                    fillColor = Qt::white;
                }
            }

            if(pen != QPen())
            {
                painter->setPen(pen);
            }
        }

        painter->fillRect(backgroundRect, rowColor);
        if(!path.isEmpty())
        {
            painter->fillPath(path.simplified(), fillColor);
            painter->drawPath(path.simplified());
        }

        //REMOVE SEPARATION LINE WHEN EXPANDED
        if(state & (QStyle::State_MouseOver | QStyle::State_Selected))
        {
            QRectF collision;

            if(index.parent().isValid())
            {
                collision = QRectF(realGeometry.x() + LEFT_MARGIN + PEN_WIDTH/2 /* next pixel*/,
                                   realGeometry.y() - PEN_WIDTH/2,
                                   realGeometry.x() + realGeometry.width() - RIGHT_MARGIN - PEN_WIDTH,
                                   PEN_WIDTH);

            }
            else if(isExpanded)
            {
                collision = QRectF(realGeometry.x() + LEFT_MARGIN + PEN_WIDTH/2 /* next pixel*/,
                                   realGeometry.y() + realGeometry.height() - PEN_WIDTH/2,
                                   realGeometry.x() + realGeometry.width() - RIGHT_MARGIN - PEN_WIDTH,
                                   PEN_WIDTH);
            }

            painter->fillRect(collision, fillColor);
        }

        painter->save();
        painter->translate(pos);

        QModelIndex editorCurrentIndex(getEditorCurrentIndex());

        bool renderDelegate(editorCurrentIndex != index);

        if(renderDelegate)
        {
            StalledIssueBaseDelegateWidget* w (getStalledIssueItemWidget(index, stalledIssueItem, geometry.size()));
            if(!w)
            {
                painter->restore();
                return;
            }

            painter->save();

            w->expand(isExpanded);
            w->setGeometry(geometry);

            auto pixmap = w->grab();
            painter->drawPixmap(0,0,w->width(), w->height(),pixmap);

            painter->restore();
        }
        else if(mEditor)
        {
            auto sourceIndex = mProxyModel->mapToSource(index);
            mCacheManager.updateEditor(sourceIndex, mEditor, stalledIssueItem);
        }

        bool drawBottomLine(false);

        if(index.parent().isValid())
        {
            drawBottomLine = true;
        }
        else
        {
            if(!isExpanded)
            {
                drawBottomLine = true;
            }
        }

        if(drawBottomLine)
        {
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setPen(QPen(Qt::black, 1));
            painter->setOpacity(0.2);
            painter->drawLine(QPoint(0 - geometry.x(), geometry.height()-1),QPoint(geometry.width(), geometry.height()-1));
        }

        painter->restore();
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

bool StalledIssueDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QWidget *StalledIssueDelegate::createEditor(QWidget*, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    auto stalledIssueItem (qvariant_cast<StalledIssueVariant>(index.data(Qt::DisplayRole)));

    if(stalledIssueItem.consultData())
    {
        mEditor = getStalledIssueItemWidget(index, stalledIssueItem);
    }

    return mEditor;
}

void StalledIssueDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    QRect geometry(option.rect);
#ifdef __APPLE__
        auto width = mView->size().width();
        width -= mView->contentsMargins().left();
        width -= mView->contentsMargins().right();
        if(mView->verticalScrollBar() && mView->verticalScrollBar()->isVisible())
        {
            width -= mView->verticalScrollBar()->width();
        }
        geometry.setWidth(std::min(width, option.rect.width()));
#endif
    editor->setGeometry(geometry);
}

bool StalledIssueDelegate::event(QEvent *event)
{
    if(auto hoverEvent = dynamic_cast<MegaDelegateHoverEvent*>(event))
    {
        if(hoverEvent->type() == QEvent::Enter)
        {
            auto headerIndex(getHeaderIndex(hoverEvent->index()));
            auto relativeIndex(getRelativeIndex(hoverEvent->index()));

            mLastHoverIndex = headerIndex;
            QTimer::singleShot(0, this, [this, relativeIndex](){
                mView->update(relativeIndex);
            });

            onHoverEnter(hoverEvent->index());
        }
        else if(hoverEvent->type() == QEvent::MouseMove)
        {
            onHoverEnter(hoverEvent->index());
        }
        else if(hoverEvent->type() == QEvent::Leave)
        {
            auto headerIndex(getHeaderIndex(hoverEvent->index()));
            if(mLastHoverIndex == headerIndex)
            {
                auto relativeIndex(getRelativeIndex(hoverEvent->index()));
                QTimer::singleShot(0, this, [this, relativeIndex](){
                    mView->update(relativeIndex);
                });

                mLastHoverIndex = QModelIndex();
            }
            onHoverLeave(hoverEvent->index());
        }
    }

    return QStyledItemDelegate::event(event);
}

bool StalledIssueDelegate::eventFilter(QObject *object, QEvent *event)
{
    if(object == mView && event->type() == QEvent::Resize)
    {
        mUpdateSizeHintTimerFromResize.start(UPDATE_SIZE_TIMER);

    }
    else if(event->type() == QEvent::MouseButtonRelease)
    {
        if(auto mouseButtonEvent = dynamic_cast<QMouseEvent*>(event))
        {
            if(mouseButtonEvent->button() == Qt::LeftButton
                    && mouseButtonEvent->modifiers() == Qt::KeyboardModifier::NoModifier)
            {
                auto viewPos = mView->mapFromGlobal(mouseButtonEvent->globalPos());
                auto index = mView->indexAt(viewPos);

                if(index.isValid() && !index.parent().isValid())
                {
                    if(auto header = dynamic_cast<StalledIssueHeader*>(mEditor.data()))
                    {
                        auto data(header->getData());
                        if(data.isValid())
                        {
                            auto currentState = mView->isExpanded(index);

                            //Allow to collapse, but when expanded, change the category
                            if(!currentState && data.consultData()->isFailed())
                            {
                                auto sourceIndex(mProxyModel->mapToSource(index));
                                //Don´t expand if we really change the tab, otherwise we need to expand it
                                if(emit goToIssue(StalledIssueFilterCriterion::FAILED_CONFLICTS, sourceIndex))
                                {
                                    return true;
                                }
                            }

                            if(header->isExpandable())
                            {
                                currentState ? mView->collapse(index) : mView->expand(index);

                                mEditor->expand(!currentState);

                                auto childIndex = index.model()->index(0, 0, index);
                                //If it is going to be expanded
                                if(!currentState)
                                {
                                    if(childIndex.isValid())
                                    {
                                        mView->scrollTo(childIndex);
                                    }
                                }

                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return QStyledItemDelegate::eventFilter(object, event);
}

void StalledIssueDelegate::destroyEditor(QWidget*, const QModelIndex&) const
{
    //Do not destroy it the editor, as it is also used to paint the row and it is saved in a cache
    mEditor = nullptr;
}

void StalledIssueDelegate::onHoverEnter(const QModelIndex &index)
{
    QModelIndex editorCurrentIndex(getEditorCurrentIndex());

    if(editorCurrentIndex != index)
    {
        onHoverLeave(index);

        mView->edit(index);
    }
}

void StalledIssueDelegate::onHoverLeave(const QModelIndex& index)
{
    //It is mandatory to close the editor, as it may be different depending on the row
    if(mEditor)
    {
        closeEditor(mEditor, QAbstractItemDelegate::EndEditHint::NoHint);

        //Small hack to avoid blinks when changing from editor to delegate paint
        //Set the editor to nullptr and update the view -> Then the delegate paints the base widget
        //before the editor is removed
        mEditor = nullptr;
    }

    mView->update(index);
}

QColor StalledIssueDelegate::getRowColor(const QModelIndex &index) const
{
    QColor rowColor;

    if(index.row()%2 == 0)
    {
        rowColor = Qt::white;
    }
    else
    {
        rowColor.setRed(0);
        rowColor.setGreen(0);
        rowColor.setBlue(0);
        rowColor.setAlphaF(0.05);
    }

    return rowColor;
}

QModelIndex StalledIssueDelegate::getEditorCurrentIndex() const
{
    if(mEditor)
    {
       return mProxyModel->mapFromSource(mEditor->getCurrentIndex());
    }

    return QModelIndex();
}

QModelIndex StalledIssueDelegate::getRelativeIndex(const QModelIndex& index) const
{
    if(index.parent().isValid())
    {
        return index.parent();
    }
    else
    {
        if(auto model = index.model())
        {
            model->index(0,0,index);
        }
    }
    return QModelIndex();
}

QModelIndex StalledIssueDelegate::getHeaderIndex(const QModelIndex &index) const
{
    return index.parent().isValid() ? index.parent() : index;
}

StalledIssueBaseDelegateWidget *StalledIssueDelegate::getStalledIssueItemWidget(const QModelIndex& proxyIndex, const StalledIssueVariant& data, const QSize& size) const
{
    StalledIssueBaseDelegateWidget* item(nullptr);

    auto finalIndex(proxyIndex);
    auto sourceIndex = mProxyModel->mapToSource(proxyIndex);
    if(sourceIndex.isValid())
    {
        finalIndex = sourceIndex;
    }

    if(finalIndex.parent().isValid())
    {
        item = mCacheManager.getStalledIssueInfoWidget(finalIndex, proxyIndex, mView->viewport(), data, size);
    }
    else
    {
        item = mCacheManager.getStalledIssueHeaderWidget(finalIndex, proxyIndex, mView->viewport(), data, size);
    }

    return item;
}
