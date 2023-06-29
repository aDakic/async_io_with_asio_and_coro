#include "ServerWindow.hpp"

namespace ui
{

    ServerWindow::ServerWindow(QWidget* parent) : QLabel(parent)
    {
        resize(800, 600);
        connect(this, &ServerWindow::requestUpdate, &ServerWindow::updateImage);
    }

    ServerWindow::~ServerWindow() { }

    void ServerWindow::asyncImageUpdate(const std::filesystem::path& imagePath) { emit requestUpdate(imagePath); }

    void ServerWindow::cancel() { QCoreApplication::quit(); }
    void ServerWindow::updateImage(const std::filesystem::path& imagePath)
    {
        QPixmap image(imagePath.c_str());

        setPixmap(std::move(image));
        setScaledContents(true);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

}  // namespace ui