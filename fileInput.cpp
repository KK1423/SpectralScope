#include "fileInput.h"

void FileInput::effectProcessor(const float *inputFrames, unsigned int &inputFrameCount,
                     float *outputFrames, unsigned int &outputFrameCount,
                     unsigned int frameChannelCount)
{
    unsigned int framesToProcess = std::min(inputFrameCount, outputFrameCount);
    for (unsigned int i = 0; i < framesToProcess * frameChannelCount; ++i)
    {
        outputFrames[i] = inputFrames[i];
    }
    inputFrameCount = framesToProcess;
    outputFrameCount = framesToProcess;

    std::vector<float> outputFrameAverages(outputFrameCount);
    for (unsigned int i = 0; i < outputFrameCount; ++i)
    {
        float sum = 0;
        for (unsigned int j = 0; j < frameChannelCount; ++j)
        {
            sum += outputFrames[i * frameChannelCount + j];
        }
        outputFrameAverages[i] = sum / frameChannelCount;
    }
    ingestSamples(outputFrameAverages.data(), outputFrameCount);
}