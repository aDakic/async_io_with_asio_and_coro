set(SOURCES main.cpp)

add_executable(client ${SOURCES})
target_link_libraries(
    client PRIVATE camera_asio::exe 
                   camera_asio::gst 
                   camera_asio::net 
                   camera_asio::dir
                   camera_asio::po)