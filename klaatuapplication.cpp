#include "klaatuapplication.h"

void KlaatuApplication::showMouse(bool show)
{
    QGuiApplication *qg = ((QGuiApplication *)(QCoreApplication::instance()));
    QCursor cursor = show ? QCursor(Qt::ArrowCursor) : QCursor(Qt::BlankCursor);
    if (qg->overrideCursor() && 0)
    {
        qg->changeOverrideCursor(cursor);
    }
    else
    {
        qg->setOverrideCursor(cursor);
    }
}
