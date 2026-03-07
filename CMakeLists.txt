cmake_minimum_required(VERSION 3.16)
project(image-viewer)
find_package(ZLIB REQUIRED)
find_package(SDL3 REQUIRED)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# This assumes the SDL source is available in vendored/SDL
# add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
# Create your game executable target as usual
add_executable(image-viewer WIN32 
./src/image-viewer.c
./src/png.c
)

# Link to the actual SDL3 library.
target_link_libraries(image-viewer PRIVATE SDL3::SDL3 ZLIB::ZLIB)




