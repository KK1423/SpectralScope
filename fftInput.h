#pragma once
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <queue>
#include <optional>
#include <array>
#include <mutex>
#include <complex>


class FFTInput {
protected:
    std::mutex mutex;
    size_t savedSampleCount = 0;
    static const size_t BUFFER_SIZE = DFT_SIZE * DECIM;
    std::array<float, BUFFER_SIZE> sampleQueue;
    static const size_t filterSize = 12;
    void ingestSamples(const float* samples, std::size_t sampleCount);

    static const std::array<float, DECIM * filterSize> sincCoefficients;

public:
    virtual ~FFTInput() = default;
    std::optional<std::array<std::complex<float>, DFT_SIZE>> getFFT();
    virtual float getTime() {return 0.0f;};
    virtual void handleEvent(const sf::Event& event) {}

};
