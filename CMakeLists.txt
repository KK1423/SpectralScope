cmake_minimum_required(VERSION 3.10)
project(FFT)

set(CMAKE_CXX_STANDARD 17)
add_executable(main)
option(ENABLE_WINDOWS_LOOPBACK "Enable Windows loopback audio capture" ON)
set(WINDOW_TITLE "SpectralScope" CACHE STRING "Application window title")
set(DFT_SIZE 512 CACHE STRING "FFT DFT size")
set(DECIM 16 CACHE STRING "FFT decimation factor")

if(ENABLE_WINDOWS_LOOPBACK)
    set(ENABLE_WINDOWS_LOOPBACK_VALUE 1)
else()
    set(ENABLE_WINDOWS_LOOPBACK_VALUE 0)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(PROJECT_SOURCES
	fft.cpp
	fftInput.cpp
	main.cpp
	micInput.cpp
	fileInput.cpp
)

if (ENABLE_WINDOWS_LOOPBACK AND WIN32)
	list(APPEND PROJECT_SOURCES loopbackInput.cpp)
endif()

target_sources(main PRIVATE ${PROJECT_SOURCES})

find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)
target_link_libraries(main PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)

if (ENABLE_WINDOWS_LOOPBACK AND WIN32)
	target_link_libraries(main PRIVATE ksuser ole32 uuid)
endif()
