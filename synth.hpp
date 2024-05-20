/*
    FM Transmitter - use Raspberry Pi as FM transmitter

    Copyright (c) 2021, Marcin Kondej
    All rights reserved.

    See https://github.com/markondej/fm_transmitter

    Redistribution and use in source and binary forms, with or without modification, are
    permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its contributors may be
    used to endorse or promote products derived from this software without specific
    prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
    SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
    TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
    WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SYNTH_HPP
#define SYNTH_HPP

#include "audio_source.hpp"
#include "sample.hpp"
#include <math.h>
#include <string>
#include <vector>

class Synth: public AudioSource
{
    public:
        Synth(bool &stop);
        virtual ~Synth();
        Synth(const Synth &) = delete;
        Synth(Synth &&) = delete;
        Synth &operator=(const Synth &) = delete;
        std::vector<Sample> GetSamples(unsigned quantity, bool &stop);
        bool SetSampleOffset(unsigned offset) { return true; }

	uint16_t GetChannels() { return 4; }
	uint32_t GetSampleRate() { return 22050; }
	uint16_t GetBitsPerSample() { return 16; }
        void process_midimessage(unsigned char byte);
        float GetNextSample();
    private:
        unsigned char midicommand;
        unsigned char midinote;
        unsigned char midivol;
};

#endif // SYNTH_HPP
