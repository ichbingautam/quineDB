add_library(quine-consensus OBJECT raft.hpp consensus.hpp)
target_include_directories(quine-consensus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
