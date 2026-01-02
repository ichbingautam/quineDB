# FindLibUring.cmake - Stub for now
# This module will look for liburing headers and library

find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
find_library(LIBURING_LIBRARY NAMES uring)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUring DEFAULT_MSG LIBURING_LIBRARY LIBURING_INCLUDE_DIR)

if(LIBURING_FOUND)
    set(LIBURING_LIBRARIES ${LIBURING_LIBRARY})
    set(LIBURING_INCLUDE_DIRS ${LIBURING_INCLUDE_DIR})
endif()
