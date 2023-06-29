#pragma once

#include <QEvent>
#include <QApplication>

namespace ui
{
    class ExitEvent : public QEvent
    {
    public:
        static QEvent::Type eventType()
        {
            static QEvent::Type exitEventType = static_cast<QEvent::Type>(QEvent::registerEventType());
            return exitEventType;
        }

        ExitEvent() : QEvent(eventType()) { }
    };


    inline bool isUIActive()
    {
        return QApplication::activeWindow() != nullptr || QApplication::activeModalWidget() != nullptr;
    }
}  // namespace ui