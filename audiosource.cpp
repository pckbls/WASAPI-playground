#include "audiosource.h"
#include <iostream>
#include <stdio.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <comdef.h>

// Documentation:
// https://msdn.microsoft.com/en-us/library/windows/desktop/dd370800(v=vs.85).aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/dd316551(v=vs.85).aspx

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

class AudioSource::Impl
{
public:
    Impl();
    ~Impl();

    void startRecording();
    void stopRecording();
    void readSamples(std::vector<float> &data);
    unsigned int channels();
    unsigned int sampleRate();

private:
    // REFERENCE_TIME time units per second and per millisecond
    const int REFTIMES_PER_SEC = 10000000;
    const int REFTIMES_PER_MILLISEC = 10000;

    void hHR(const HRESULT& hr);

    IMMDeviceEnumerator *pEnumerator;
    IMMDevice *pDevice;
    IAudioClient *pAudioClient;
    IAudioCaptureClient *pCaptureClient;

    WAVEFORMATEX *pwfx;

    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;

    UINT32 bufferFrameCount;

    std::vector<float> buffer;
};

AudioSource::AudioSource() : impl(new AudioSource::Impl()) {}
AudioSource::~AudioSource() {}
void AudioSource::startRecording() { this->impl->startRecording(); }
void AudioSource::stopRecording() { this->impl->stopRecording(); }
unsigned int AudioSource::channels() { return this->impl->channels(); }
unsigned int AudioSource::sampleRate() { return this->impl->sampleRate(); }
void AudioSource::readSamples(std::vector<float>& data) { return this->impl->readSamples(data); }

AudioSource::Impl::Impl()
    :
    pDevice(nullptr),
    pAudioClient(nullptr),
    pCaptureClient(nullptr),
    pwfx(nullptr)
{
    hHR(CoInitialize(nullptr));
    hHR(CoCreateInstance(
        CLSID_MMDeviceEnumerator,
        nullptr,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        (void**)&this->pEnumerator
    ));

    hHR(pEnumerator->GetDefaultAudioEndpoint(
        eRender, // Important for loopback recording!
        eConsole,
        &this->pDevice
    ));

    hHR(this->pDevice->Activate(
        IID_IAudioClient,
        CLSCTX_ALL,
        nullptr,
        (void**)&this->pAudioClient
    ));

    hHR(this->pAudioClient->GetMixFormat(&this->pwfx));

    hHR(this->pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK, // Important for loopback recording!
        this->hnsRequestedDuration,
        0,
        this->pwfx,
        nullptr
    ));

    // TODO: PCM or IEEE_FLOAT would be soo much easier...
    if (this->pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE)
    {
        throw "Cannot handle that";
    }

    if (this->pwfx->wBitsPerSample != 8)
    {
        throw "We expect 8 bits per sample";
    }

    // Get the size of the allocated buffer and calculate the actual duration of it.
    hHR(this->pAudioClient->GetBufferSize(&this->bufferFrameCount));
    this->hnsActualDuration = REFTIMES_PER_SEC * this->bufferFrameCount / this->pwfx->nSamplesPerSec;

    std::cout << "Buffer frame count: " << this->bufferFrameCount << std::endl;
    this->buffer = std::vector<float>(this->bufferFrameCount);

    // Obtain a reference to the capture client.
    hHR(pAudioClient->GetService(
        IID_IAudioCaptureClient,
        (void**)&this->pCaptureClient
    ));
}

void AudioSource::Impl::hHR(const HRESULT& hr)
{
    if (!FAILED(hr))
        return;

    _com_error err(hr);
    LPCTSTR errMsg = err.ErrorMessage();
    std::cerr << errMsg << std::endl;
    throw "Foo"; // TODO
}

AudioSource::Impl::~Impl()
{
    CoTaskMemFree(this->pwfx);
    if (this->pEnumerator != nullptr) this->pEnumerator->Release();
    if (this->pDevice != nullptr) this->pDevice->Release();
    if (this->pAudioClient != nullptr) this->pAudioClient->Release();
    if (this->pCaptureClient != nullptr) this->pCaptureClient->Release();
}

void AudioSource::Impl::startRecording()
{
    hHR(this->pAudioClient->Start());
}

void AudioSource::Impl::stopRecording()
{
    hHR(this->pAudioClient->Stop());
}

unsigned int AudioSource::Impl::channels()
{
    return this->pwfx->nChannels;
}

unsigned int AudioSource::Impl::sampleRate()
{
    return this->pwfx->nSamplesPerSec;
}

void AudioSource::Impl::readSamples(std::vector<float>& data)
{
    BYTE *pData;
    UINT32 numFramesAvailable;
    UINT32 packetLength = 0;
    DWORD flags;

    for (size_t bytes_copied = 0; bytes_copied < data.size(); /* nothing */)
    {
        // Sleep for half the buffer duration.
        Sleep(this->hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

        hHR(this->pCaptureClient->GetNextPacketSize(&packetLength));
        std::cout << "Packet length: " << packetLength << std::endl;
        while (packetLength != 0)
        {
            // Get the available data in the shared buffer.
            hHR(this->pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr));
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                pData = nullptr; // Silence?

            // TODO: Handle data!
            std::cout << "Read " << numFramesAvailable << " bytes" << std::endl;

            hHR(this->pCaptureClient->ReleaseBuffer(numFramesAvailable));
            hHR(this->pCaptureClient->GetNextPacketSize(&packetLength));
        }

        break; // TODO
    }
}
