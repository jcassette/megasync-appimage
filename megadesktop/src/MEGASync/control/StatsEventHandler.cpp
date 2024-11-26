#include "StatsEventHandler.h"

StatsEventHandler::StatsEventHandler(mega::MegaApi* megaApi, QObject* parent)
    : QObject(parent)
    , mMegaApi(megaApi)
    , mViewID(nullptr)
    , mCurrentView(nullptr)
    , mInfoDialogVisible(false)
    , mUpdateViewID(true)
    , mLastInfoDialogEventSent(true)
{
    if(megaApi)
    {
        mViewID = mMegaApi->generateViewId();
    }
}

bool StatsEventHandler::eventFilter(QObject* obj, QEvent* event)
{
    if(mInfoDialogVisible)
    {
        if(!MegaSyncApp->isInfoDialogVisible())
        {
            mInfoDialogVisible = false;
            mLastInfoDialogEventSent = false;
            mUpdateViewID = true;
        }
    }
    else
    {
        if(MegaSyncApp->isInfoDialogVisible())
        {
            mInfoDialogVisible = true;
            mUpdateViewID = true;
            mLastInfoDialogEventSent = true;
        }
        else if(event->type() == QEvent::WindowActivate
                   || event->type() == QEvent::FocusIn
                   || event->type() == QEvent::FocusOut)
        {
            if(mCurrentView != obj)
            {
                mCurrentView = obj;
                mUpdateViewID = true;
            }
        }
    }

    return QObject::eventFilter(obj, event);
}
