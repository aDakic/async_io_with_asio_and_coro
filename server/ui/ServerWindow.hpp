#pragma once

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <filesystem>

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
        void requestQuit();

    private slots:
        void updateImage(const std::filesystem::path& imagePath);
    };
}  // namespace ui
