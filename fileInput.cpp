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