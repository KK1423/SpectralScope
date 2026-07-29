#include "fftInput.h"
#include "math_constants.h"
#include <stdexcept>
#include <vector>
#include "fft.h"

using namespace sf;

// sinc filter coefficients
const std::array<float, DECIM*FFTInput::filterSize> FFTInput::sincCoefficients = [](){
    std::array <float, DECIM*filterSize> ret;
    const float excessBandwidth = 1.5;
    for (size_t i = 0; i < DECIM*filterSize; ++i) {
        float x = (static_cast<float>(i) - DECIM*filterSize/2);
        if (x == 0.0f) {
            ret[i] = excessBandwidth * 1.f/DECIM;
        } else {
            ret[i] = std::sin(excessBandwidth * fft::PI * x / DECIM) / (fft::PI * x);
        }
    }
    return ret;
}();

std::optional<std::array<std::complex<float>, DFT_SIZE>> FFTInput::getFFT() {
    std::lock_guard<std::mutex> lock(mutex);
    if (savedSampleCount < BUFFER_SIZE)
    {
        return {};
    }
    std::array<float, DFT_SIZE> fftData = {{0}};
    for (size_t i = 0; i < DFT_SIZE; i ++) {
        #ifdef ALIAS
        for (size_t j = 0; j < DECIM; j++)
        {
            fftData[i] += sampleQueue[i * DECIM + j] / DECIM;
        }
        continue;

        #endif
        for (size_t j = 0; j < sincCoefficients.size(); j++)
        {
            size_t cIndex = (i * DECIM
                           + j - sincCoefficients.size()/2
                           + sampleQueue.size()) % sampleQueue.size();
            fftData[i] += sampleQueue[cIndex] * sincCoefficients[j];
        }
    }

    return DFT(fftData);
}

void FFTInput::ingestSamples(const float* samples, std::size_t sampleCount) {
    std::lock_guard<std::mutex> lock(mutex);
    if (sampleCount > BUFFER_SIZE) {
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            sampleQueue[i] = samples[sampleCount - BUFFER_SIZE + i];
        }
        savedSampleCount = BUFFER_SIZE;
        return;
    }

    if (savedSampleCount + sampleCount > BUFFER_SIZE) {
        for (size_t i = 0, j = savedSampleCount + sampleCount - BUFFER_SIZE; j < savedSampleCount; ++i, ++j) {
            sampleQueue[i] = sampleQueue[j];
        }
        for (size_t i = 0; i < sampleCount; ++i) {
            sampleQueue[BUFFER_SIZE - sampleCount + i] = samples[i];
        }
        savedSampleCount = BUFFER_SIZE;
    } else {
        for (size_t i = 0; i < sampleCount; ++i) {
            sampleQueue[savedSampleCount + i] = samples[i];
        }
        savedSampleCount += sampleCount;
    }
}
