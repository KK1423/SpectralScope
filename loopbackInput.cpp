#ifdef _WIN32
#include "loopbackInput.h"
#include <Windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Mmreg.h>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdint>

static void downmixToMono(const BYTE* data, WAVEFORMATEX* format, UINT32 frames, std::vector<float>& output)
{
    const UINT32 channelCount = format->nChannels;
    output.resize(frames);

    for (UINT32 frame = 0; frame < frames; ++frame) {
        float sum = 0.0f;
        const BYTE* framePtr = data + frame * format->nBlockAlign;
        const float* frameData = reinterpret_cast<const float*>(framePtr);
        for (UINT32 channel = 0; channel < channelCount; ++channel) {
            sum += frameData[channel];
        }
        output[frame] = sum / static_cast<float>(channelCount);
    }
}

static void safeRelease(IUnknown** ptr)
{
    if (*ptr) {
        (*ptr)->Release();
        *ptr = nullptr;
    }
}

LoopbackInput::LoopbackInput()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to initialize COM for WASAPI loopback");
    }

    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr) || !enumerator) {
        CoUninitialize();
        throw std::runtime_error("Failed to create IMMDeviceEnumerator");
    }

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    enumerator->Release();
    if (FAILED(hr) || !device) {
        CoUninitialize();
        throw std::runtime_error("Failed to get default audio render endpoint");
    }

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audioClient));
    if (FAILED(hr) || !audioClient) {
        device->Release();
        CoUninitialize();
        throw std::runtime_error("Failed to activate IAudioClient");
    }

    waveFormat = (WAVEFORMATEX*) CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if (!waveFormat) {
        closeAudioClient();
        throw std::runtime_error("Failed to allocate memory for WAVEFORMATEX");
    }

    WAVEFORMATEXTENSIBLE &wfx = *reinterpret_cast<WAVEFORMATEXTENSIBLE*>(waveFormat);

    wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels = 1;
    wfx.Format.nSamplesPerSec = 48000;
    wfx.Format.wBitsPerSample = 32;
    wfx.Format.nBlockAlign = (wfx.Format.nChannels * wfx.Format.wBitsPerSample) / 8;
    wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    wfx.Samples.wValidBitsPerSample = 32;
    wfx.dwChannelMask = KSAUDIO_SPEAKER_MONO;
    wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

    REFERENCE_TIME hnsBufferDuration = 250000; // 25 ms
    captureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!captureEvent) {
        closeAudioClient();
        throw std::runtime_error("Failed to create WASAPI capture event");
    }

    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_LOOPBACK |
                                 AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                                 AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
                                 hnsBufferDuration,
                                 0,
                                 waveFormat,
                                 NULL);
    if (FAILED(hr)) {
        closeAudioClient();
        CloseHandle(captureEvent);
        captureEvent = nullptr;
        throw std::runtime_error("Failed to initialize WASAPI audio client for loopback");
    }

    hr = audioClient->SetEventHandle(captureEvent);
    if (FAILED(hr)) {
        closeAudioClient();
        CloseHandle(captureEvent);
        captureEvent = nullptr;
        throw std::runtime_error("Failed to set WASAPI capture event handle");
    }

    hr = audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureClient));
    if (FAILED(hr) || !captureClient) {
        closeAudioClient();
        throw std::runtime_error("Failed to get IAudioCaptureClient");
    }

    hr = audioClient->Start();
    if (FAILED(hr)) {
        closeAudioClient();
        throw std::runtime_error("Failed to start WASAPI audio client");
    }

    running = true;
    captureThread = std::thread(&LoopbackInput::captureThreadFunc, this);
}

LoopbackInput::~LoopbackInput()
{
    running = false;
    if (captureThread.joinable()) {
        captureThread.join();
    }

    if (audioClient) {
        audioClient->Stop();
    }
    closeAudioClient();
    if (waveFormat) {
        CoTaskMemFree(waveFormat);
        waveFormat = nullptr;
    }
    CoUninitialize();
}

void LoopbackInput::handleEvent(const sf::Event& event)
{
    if (const auto* keypressEvent = event.getIf<sf::Event::KeyPressed>()) {
        using sf::Keyboard::Key;
        switch (keypressEvent->code)
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

void LoopbackInput::captureThreadFunc()
{

    while (running) {
        if (captureEvent) {
            DWORD wait = WaitForSingleObject(captureEvent, 2000);
            if (wait != WAIT_OBJECT_0) {
                if (wait == WAIT_TIMEOUT) {
                    continue;
                }
                break;
            }
        }

        UINT32 packetLength = 0;
        HRESULT hr = captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            continue;
        }

        if (packetLength == 0) {
            continue;
        }

        BYTE* data = nullptr;
        UINT32 numFrames = 0;
        DWORD flags = 0;
        hr = captureClient->GetBuffer(&data, &numFrames, &flags, NULL, NULL);
        if (FAILED(hr)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (numFrames > 0) {
            std::vector<BYTE> rawBuffer;
            UINT32 frameSize = waveFormat->nBlockAlign;
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                rawBuffer.resize(static_cast<size_t>(numFrames) * frameSize);
                std::fill(rawBuffer.begin(), rawBuffer.end(), 0);
                data = rawBuffer.data();
            }

            if (frameSize == sizeof(float)) {
                float* samples = reinterpret_cast<float*>(data);
                for (size_t i = 0; i < numFrames; ++i) {
                    samples[i] *= gain;
                }

                ingestSamples(samples, numFrames);
            } else {
                std::vector<float> floatBuf;
                downmixToMono(data, waveFormat, numFrames, floatBuf);
                for (auto& sample : floatBuf) {
                    sample *= gain;
                }

                ingestSamples(floatBuf.data(), floatBuf.size());
            }

        }

        captureClient->ReleaseBuffer(numFrames);
    }
}

void LoopbackInput::closeAudioClient()
{
    if (captureClient) {
        captureClient->Release();
        captureClient = nullptr;
    }
    if (captureEvent) {
        CloseHandle(captureEvent);
        captureEvent = nullptr;
    }
    if (audioClient) {
        audioClient->Release();
        audioClient = nullptr;
    }
    if (device) {
        device->Release();
        device = nullptr;
    }
}

#endif
