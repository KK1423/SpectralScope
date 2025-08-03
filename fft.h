#include <stdint.h>
#include <vector>
#include <array>
#include <optional>
#include <complex>

std::array<std::complex<float>, DFT_SIZE> DFT(const std::array<float, DFT_SIZE>& input);
