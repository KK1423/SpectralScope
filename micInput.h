#pragma once
#include "fftInput.h"
#include <SFML/Audio.hpp>
#include <vector>

class MicInput : public FFTInput, public sf::SoundRecorder {
public:
    MicInput();
    virtual ~MicInput();

    void handleEvent(const sf::Event& event) override;
protected:
    float gain = 1.0f;
    virtual bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount) override;
    std::vector<float> floatSamples;
};
