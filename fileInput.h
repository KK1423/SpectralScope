#pragma once
#include "fftInput.h"
#include <memory>
#include <SFML/Audio.hpp>

class FileInput : public FFTInput
{
public:
    explicit FileInput(std::unique_ptr<sf::SoundStream> &&source) : source(std::move(source))
    {
        if (!this->source)
        {
            throw std::runtime_error("Failed to create FileInput: source is null");
        }

        this->source->setEffectProcessor(std::bind(&FileInput::effectProcessor, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        this->source->play();
    }
    virtual ~FileInput() = default;
    FileInput(FileInput &&) = delete;

    FileInput &operator=(FileInput &&) = delete;

    FileInput(const FileInput &) = delete;
    FileInput &operator=(const FileInput &) = delete;
    std::unique_ptr<sf::SoundStream> source;

    std::vector<float> sampleBuffer;
    std::size_t samplesProcessed = 0;

    void effectProcessor(const float *inputFrames, unsigned int &inputFrameCount,
                                 float *outputFrames, unsigned int &outputFrameCount,
                                 unsigned int frameChannelCount);
};
