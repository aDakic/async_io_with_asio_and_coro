set(SOURCES main.cpp)

add_executable(Client ${SOURCES})
target_link_libraries(
 Client PRIVATE recorder_asio::exe 
                recorder_asio::gst 
                recorder_asio::net 
                recorder_asio::dir
                recorder_asio::po)