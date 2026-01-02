add_library(quine-network
    tcp_server.cpp
    connection.cpp
    resp_parser.cpp
)

target_link_libraries(quine-network PUBLIC
    quine-core
)

target_include_directories(quine-network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
