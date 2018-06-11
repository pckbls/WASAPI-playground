#pragma once

#include <vector>
#include <memory>

class AudioSource
{
public:
    AudioSource();
    AudioSource(const AudioSource& other) = delete;
    ~AudioSource();

    void startRecording();
    void stopRecording();
    void readSamples(std::vector<float> &data);
    unsigned int channels();
    unsigned int sampleRate();

private:
    // We use the PIMPL idiom here to hide the implementation details of the AudioSource class.
    // Otherwise we would have to include various WINAPI headers here and therefore
    // wherever we include our audiosource.h we would pollute the global namespace with
    // WINAPI definitions.
    class Impl;
    std::unique_ptr<Impl> impl;
};
