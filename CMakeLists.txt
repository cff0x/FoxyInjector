cmake_minimum_required(VERSION 3.8)
project("FoxyInjector" C CXX)

# set c++ language standard to 20
set(CMAKE_CXX_STANDARD 20)

# disable shared library building
set(BUILD_SHARED_LIBS OFF)
set(BUILD_SHARED OFF)

# add qt library
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)

#set(QT_DIR $ENV{Qt6_HOME})
#set(CMAKE_PREFIX_PATH $ENV{Qt6_HOME}/lib/cmake)

#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PATHS ${QT_DIR})
#find_package(Qt6 REQUIRED COMPONENTS Widgets PATHS ${QT_DIR})

# add sdl library
set(SDL_DIR "external/SDL")
add_subdirectory("${SDL_DIR}")


# add libxmp library (unneccessary options disabled)
set(LIBXMP_DIR "external/libxmp")
set(LIBXMP_INCLUDE_DIR "${LIBXMP_DIR}/include")
set(LIBXMP_CORE_PLAYER ON)
set(LIBXMP_NO_PROWIZARD ON)
set(LIBXMP_NO_DEPACKERS ON)

add_definitions(
	-DBUILDING_STATIC
)
add_subdirectory("${LIBXMP_DIR}")
#target_include_directories (libxmp PUBLIC "${LIBXMP_DIR}/src" "${LIBXMP_INCLUDE_DIR}")

# add cxxopts library (header-only)
set(CXXOPTS_DIR "external/cxxopts")
include_directories("${CXXOPTS_DIR}/include")


# add lzma2 library (we build it here because it has no CMakeLists.txt)
set(LZMA2_DIR "external/fast-lzma2")
add_library (lzma2 STATIC
	"${LZMA2_DIR}/dict_buffer.c" "${LZMA2_DIR}/fl2_common.c" "${LZMA2_DIR}/fl2_compress.c"
	"${LZMA2_DIR}/fl2_decompress.c" "${LZMA2_DIR}/fl2_pool.c" "${LZMA2_DIR}/fl2_threading.c"
	"${LZMA2_DIR}/lzma2_dec.c" "${LZMA2_DIR}/lzma2_enc.c" "${LZMA2_DIR}/radix_bitpack.c"
	"${LZMA2_DIR}/radix_mf.c" "${LZMA2_DIR}/radix_struct.c" "${LZMA2_DIR}/range_enc.c" 
	"${LZMA2_DIR}/util.c" "${LZMA2_DIR}/xxhash.c")
target_include_directories(lzma2 PUBLIC "${LZMA2_DIR}")


# add shared include dir 
include_directories("foxy-shared")


# add module library
add_library (Hekk STATIC MODULE "foxy-module/hekk.cpp" "foxy-module/console.cpp" "foxy-module/xm.h")
target_link_libraries(Hekk libxmp::xmp_static SDL2::SDL2main SDL2::SDL2-static)

# add console launcher
add_executable (FoxyInjector "foxy-launcher/launcher.cpp" )
target_link_libraries(FoxyInjector lzma2)
#target_include_directories(FoxyInjector PRIVATE ${QT_DIR}/include)
#target_link_libraries(FoxyInjector PRIVATE Qt6::Widgets)
