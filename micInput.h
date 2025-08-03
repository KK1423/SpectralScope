#pragma once
#include "fftInput.h"
#include <SFML/Audio.hpp>

class MicInput : public FFTInput, public sf::SoundRecorder {
public:
    MicInput();
    virtual ~MicInput();
protected:
    virtual bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount) override;
};
