#include <iostream>
#include <vector>
#include "audiosource.h"

int main()
{
    try
    {
        AudioSource source;
        std::vector<float> data(512);

        source.startRecording();
        source.readSamples(data);
        source.stopRecording();
    }
    catch (const char* e)
    {
        std::cerr << e << std::endl;
        return -1;
    }

    return 0;
}
