#Read version from file
file(READ "${CMAKE_CURRENT_LIST_DIR}/version.txt" BUILD_VERSION)
string(STRIP "${BUILD_VERSION}" BUILD_VERSION)
message(STATUS "erd_depth version is ${BUILD_VERSION}")
