#include "fileInput.h"

void FileInput::effectProcessor(const float *inputFrames, unsigned int &inputFrameCount,
                                float *outputFrames, unsigned int &outputFrameCount,
                                unsigned int frameChannelCount)
{
    unsigned int framesToProcess = std::min(inputFrameCount, outputFrameCount);

    for (unsigned int i = 0; i < inputFrameCount; ++i)
    {
        float sum = 0;
        for (unsigned int j = 0; j < frameChannelCount; ++j)
        {
            sum += inputFrames[i * frameChannelCount + j];
        }
        outputFrames[i] = sum / frameChannelCount;
    }
    ingestSamples(outputFrames, outputFrameCount);

    for (unsigned int i = 0; i < framesToProcess * frameChannelCount; ++i)
    {
        outputFrames[i] = inputFrames[i];
    }
    inputFrameCount = framesToProcess;
    outputFrameCount = framesToProcess;
}

void FileInput::handleEvent(const sf::Event &event)
{
    if (event.is<sf::Event::KeyPressed>())
    {
        auto keypressEvent = *event.getIf<sf::Event::KeyPressed>();
        using sf::Keyboard::Key;
        switch (keypressEvent.code)
        {
        case Key::Space:
            if (source->getStatus() == sf::SoundStream::Status::Playing)
            {
                source->pause();
            }
            else
            {
                source->play();
            }
            break;
        case Key::Left:
        case Key::Right:
        {
            if (source->getStatus() != sf::SoundStream::Status::Playing)
            {
                source->play();
            }
            auto currentTime = source->getPlayingOffset();
            int seekSize = keypressEvent.control ? 30 : 5;
            if (keypressEvent.code == Key::Left)
            {
                currentTime -= sf::seconds(seekSize);
            }
            else
            {
                currentTime += sf::seconds(seekSize);
            }
            if (currentTime < sf::Time::Zero)
            {
                currentTime = sf::Time::Zero;
            }
            else if (currentTime > source->getDuration())
            {
                source->stop();
                break;
            }
            source->setPlayingOffset(currentTime);
        }
        break;
        default:
            break;
        }
    }
}