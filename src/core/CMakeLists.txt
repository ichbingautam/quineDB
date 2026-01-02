add_library(quine-core
    io_context.cpp
)

target_link_libraries(quine-core PUBLIC
    LibUring::LibUring
)

target_include_directories(quine-core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)
