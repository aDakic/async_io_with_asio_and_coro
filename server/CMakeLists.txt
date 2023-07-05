set(SOURCES main.cpp ui/ServerWindow.cpp ui/ServerWindow.hpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(server ${SOURCES})
target_compile_features(server PUBLIC cxx_std_20)

find_package(Qt5
    REQUIRED COMPONENTS Core Gui Widgets)

target_link_libraries(
        server PRIVATE camera_asio::exe 
                       camera_asio::net
                       camera_asio::dir
                       camera_asio::po)

target_link_libraries(
        server PRIVATE Qt5::Core
                       Qt5::Gui
                       Qt5::Widgets)