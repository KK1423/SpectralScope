#include "micInput.h"

MicInput::MicInput() {
    std::string device = getDefaultDevice();
    if (device.empty())
    {
        throw std::runtime_error("No default audio capture device found");
    }
    setDevice(device);
    setChannelCount(1);
    if (!start(48000)) {
        throw std::runtime_error("Failed to start FFTInput");
    }
}

MicInput::~MicInput() {
    stop();
}

bool MicInput::onProcessSamples(const std::int16_t* samples, std::size_t sampleCount)
{
    std::vector<float> floatSamples(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        floatSamples[i] = static_cast<float>(samples[i] >> 8) / 256.0f;
    }
    ingestSamples(floatSamples.data(), sampleCount);
    return true;
}
