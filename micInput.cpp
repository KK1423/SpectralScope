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

void MicInput::handleEvent(const sf::Event& event)
{
    if (event.is<sf::Event::KeyPressed>()) {
        auto keypressEvent = *event.getIf<sf::Event::KeyPressed>();
        using sf::Keyboard::Key;
        switch (keypressEvent.code)
        {
        case Key::Up:
            gain *= 1.1f;
            break;
        case Key::Down:
            gain /= 1.1f;
            break;
        
        default:
            break;
        }
    }
}

bool MicInput::onProcessSamples(const std::int16_t* samples, std::size_t sampleCount)
{
    floatSamples.resize(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        floatSamples[i] = gain * static_cast<float>(samples[i] >> 8) / 256.0f;
    }
    ingestSamples(floatSamples.data(), sampleCount);
    return true;
}
