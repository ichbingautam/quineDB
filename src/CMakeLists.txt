# Main entry point
add_executable(quine-server main.cpp)

# Link core libraries (to be created in subdirs)
# target_link_libraries(quine-server PRIVATE quine-core quine-network)
target_link_libraries(quine-server PRIVATE Threads::Threads quine-core quine-network quine-storage)

# Subdirectories for libraries
# add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(network)
add_subdirectory(storage) # [NEW] Storage Layer
# add_subdirectory(storage)
# add_subdirectory(consensus)
# add_subdirectory(api)
