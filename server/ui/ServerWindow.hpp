#pragma once

#include <QLabel>
#include <QMainWindow>
#include <filesystem>

#include "Events.hpp"

namespace ui
{
    class ServerWindow : public QLabel
    {
        Q_OBJECT
    public:
        ServerWindow(QWidget* parent = nullptr);
        ~ServerWindow();

        void asyncImageUpdate(const std::filesystem::path& imagePath);
        void cancel(); //FIXME

    signals:
        void requestUpdate(const std::filesystem::path& imagePath);

    private slots:
        void updateImage(const std::filesystem::path& imagePath);
    };
}  // namespace ui
