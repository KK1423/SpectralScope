#include "config.h"
#include <stdint.h>
#include <vector>
#include <array>
#include <optional>
#include <complex>

#include <cmath>
#include "math_constants.h"

using namespace std;

inline uint64_t bitreverse(uint64_t x)
{
    // Reverse the bits of a 64-bit integer
    x = ((x & 0x5555555555555555ULL) << 1) | ((x & 0xAAAAAAAAAAAAAAAAULL) >> 1);
    x = ((x & 0x3333333333333333ULL) << 2) | ((x & 0xCCCCCCCCCCCCCCCCULL) >> 2);
    x = ((x & 0x0F0F0F0F0F0F0F0FULL) << 4) | ((x & 0xF0F0F0F0F0F0F0F0ULL) >> 4);
    x = ((x & 0x00FF00FF00FF00FFULL) << 8) | ((x & 0xFF00FF00FF00FF00ULL) >> 8);
    x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x & 0xFFFF0000FFFF0000ULL) >> 16);
    x = (x << 32) | (x >> 32);
    return x;
}

array<complex<float>, DFT_SIZE> twiddle = []()
{
    array<complex<float>, DFT_SIZE> factors;
    for (size_t i = 0; i < DFT_SIZE; ++i)
    {
        factors[i] = std::exp(-2.0f * fft::PI * std::complex<float>(0, 1) * static_cast<float>(i) / static_cast<float>(DFT_SIZE));
    }
    return factors;
}();

array<float, DFT_SIZE> cos_window = []()
{
    array<float, DFT_SIZE> window;
    for (size_t i = 0; i < DFT_SIZE; ++i)
    {
        float f = 2 * fft::PI * static_cast<float>(i) / DFT_SIZE;
        // blackman-nuttall
        //window[i] = 0.3636 - 0.4892 * cos(f) + 0.1366 * cos(2 * f) - 0.01064 * cos(3 * f);
        // flat top
        window[i] = 0.2116 - 0.4166 * cos(f) + 0.2773 * cos(2 * f) - 0.0836 * cos(3 * f) + 0.00694 * cos(4 * f);
        // hann
        //window[i] = 0.5f * (1 - cos(f));
        // sine window
        //window[i] = 0.5f * sin(f / 2);
        // rect
        //window[i] = 1.0f;
        // none 
        //window[i] = 1.0;
    }
    return window;
}();

constexpr size_t N = []()
{
    size_t n = 0;
    while ((1 << n) < DFT_SIZE)
        n++;
    return n;
}();

array<complex<float>, DFT_SIZE> DFT(const array<float, DFT_SIZE> &input)
{
    array<complex<float>, DFT_SIZE> buffer;

    for (size_t i = 0; i < DFT_SIZE; ++i)
    {
        size_t reversed_index = bitreverse(i) >> (64 - N);
        buffer[reversed_index] = cos_window[i] * input[i];
    }

    for (size_t n = 1; n <= N; n++)
    {
        size_t i = 1 << n;
        for (size_t j = 0; j < DFT_SIZE; j += i)
        {
            for (size_t k = 0, l = i / 2; l < i; k++, l++)
            {
                complex<float> t = buffer[j + l] * twiddle[(k << (N - n)) & (1 << N) - 1];
                complex<float> u = buffer[j + k];
                buffer[j + k] = u + t;
                buffer[j + l] = u - t;
            }
        }
    }

    return buffer;
}
