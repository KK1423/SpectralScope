#pragma once
#ifdef _WIN32
#include "fftInput.h"
#include <Windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <thread>
#include <atomic>
#include <vector>

class LoopbackInput : public FFTInput {
public:
    LoopbackInput();
    virtual ~LoopbackInput();

    void handleEvent(const sf::Event& event) override;
protected:
    void captureThreadFunc();
    void closeAudioClient();

    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    IMMDevice* device = nullptr;
    HANDLE captureEvent = nullptr;
    std::thread captureThread;
    std::atomic<bool> running{false};
    float gain = 1.0f;
    WAVEFORMATEX* waveFormat = nullptr;
};
#endif
