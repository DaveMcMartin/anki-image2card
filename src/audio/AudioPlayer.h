#pragma once

#include <vector>
#include <string>
#include <memory>

namespace Image2Card::Audio
{
    class AudioPlayer
    {
    public:
        AudioPlayer();
        ~AudioPlayer();

        AudioPlayer(const AudioPlayer&) = delete;
        AudioPlayer& operator=(const AudioPlayer&) = delete;

        bool Play(const std::vector<unsigned char>& data);

        void Stop();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
