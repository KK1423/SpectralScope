cmake_minimum_required(VERSION 3.10)
project(FFT)

set(CMAKE_CXX_STANDARD 17)
add_executable(main)
target_sources(main PRIVATE fft.cpp fftInput.cpp main.cpp micInput.cpp fileInput.cpp)
target_compile_definitions(main PRIVATE DFT_SIZE=512 DECIM=16)

find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)
target_link_libraries(main PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
