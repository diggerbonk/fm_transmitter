/*
    Synth - Soft Synthesizer for Raspberry Pi 

    Copyright (c) 2021, Trent McNair
    All rights reserved.
*/

#include "synth.hpp"

#include <alsa/asoundlib.h>
#include <chrono>
#include <cstring>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#define MAX_CHANNELS 8

static volatile int midinotes[MAX_CHANNELS] = { 0, 0, 0, 0 };
static volatile unsigned short midivolumes[MAX_CHANNELS] = {0, 0, 0, 0};
static volatile uint16_t synthPos = 0;
static pthread_t thread_id;
static const char *port_name = "hw:2,0,0";
static int ignore_active_sensing = 1;
static int timeout;
static int stop;
static snd_rawmidi_t *input, **inputp;

const uint16_t note_freq[] = {
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 
     16,   17,   18,   19,   21,   22,   23,   25,   26,   28,   29,   31,
     33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62, 
     65,   69,   73,   78,   82,   87,   93,   98,  104,  110,  117,  123, 
    131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247, 
    262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494, 
    523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
   1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
   2093, 2218, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
   4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
   8372, 8870, 9397, 9956,10548,11175,11840,12544   
};


static void error(const char *format, ...)
{
        va_list ap;

        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        putc('\n', stderr);
}



void Synth::process_midimessage(unsigned char byte)
{
        static enum {
                STATE_UNKNOWN,
                STATE_1PARAM,
                STATE_1PARAM_CONTINUE,
                STATE_2PARAM_1,
                STATE_2PARAM_2,
                STATE_2PARAM_1_CONTINUE,
                STATE_SYSEX
        } state = STATE_UNKNOWN;

        if (byte >= 0xf0) {
                switch (byte) {
                case 0xf0:
                        state = STATE_SYSEX;
                        break;
                case 0xf1:
                case 0xf3:
                        state = STATE_1PARAM;
                        break;
                case 0xf2:
                        state = STATE_2PARAM_1;
                        break;
                case 0xf4:
                case 0xf5:
                case 0xf6:
                        state = STATE_UNKNOWN;
                        break;
                case 0xf7:
                        state = STATE_UNKNOWN;
                        break;
                }
        } else if (byte >= 0x80) {
                if (byte >= 0xc0 && byte <= 0xdf) {
                        state = STATE_1PARAM;
                }
                else  {
                        state = STATE_2PARAM_1;
                        midicommand = byte;
                }
        } else /* b < 0x80 */ {
                int running_status = 0;
                switch (state) {
                case STATE_1PARAM:
                        state = STATE_1PARAM_CONTINUE;
                        break;
                case STATE_1PARAM_CONTINUE:
                        running_status = 1;
                        break;
                case STATE_2PARAM_1:
                        state = STATE_2PARAM_2;
                        midinote = byte;
                        break;
                case STATE_2PARAM_2:
                        state = STATE_2PARAM_1_CONTINUE;
                        midivol = byte;
                        break;
                case STATE_2PARAM_1_CONTINUE:
                        running_status = 1;
                        state = STATE_2PARAM_2;
                        break;
                default:
                        break;
                }
                if (running_status)
                        fputs("\n  ", stdout);
        }

        if (state == 5) {
            if (midicommand == 155) {
                int freeChannel = -1;
                for (int i=0; i<GetChannels(); i++) {
                  if (midinotes[i] == 0 ) freeChannel = i;
                  else if (midinotes[i] == midinote) {
                     freeChannel = -1;
                     break;
                  }
                }
                if (freeChannel >= 0) {
                     midinotes[freeChannel] = midinote;
                     midivolumes[freeChannel] = midivol * 256;
                     std::cout << "midinote on:  " << (int)midinote << ", channel: " << freeChannel << ", volume: " << (int)midivol << std::endl;
                }
            }
            else {
                for (int i=0; i<GetChannels(); i++) {
                  if (midinotes[i] == midinote) {
                    midinotes[i] = 0;
                    midivolumes[i] = 0;
                    std::cout << "midinote off: " << (int)midinote << ", channel: " << i << std::endl;
                    break;
                  }
               }
            }
        }
}

//static void sig_handler(int dummy)
//{
//        stop = 1;
//}


void *synthThread(void *args)
{
    Synth * synth = (Synth*)args;
    int err = 0;

    inputp = &input;

        if ((err = snd_rawmidi_open(inputp, NULL, port_name, SND_RAWMIDI_NONBLOCK)) < 0) {
                error("cannot open port \"%s\": %s", port_name, snd_strerror(err));
                return 0;
        }

        snd_rawmidi_params_t *params;
        snd_rawmidi_params_malloc(&params);
        snd_rawmidi_params_current(input,params);
        std::cout << "default ring buffer size: " << snd_rawmidi_params_get_buffer_size(params) << std::endl;
        snd_rawmidi_params_set_buffer_size(input,params,32);
        snd_rawmidi_params_set_avail_min(input, params, 1);
        snd_rawmidi_params(input,params);
        snd_rawmidi_params_free(params);

        snd_rawmidi_params_malloc(&params);
        snd_rawmidi_params_current(input,params);
        std::cout << "new ring buffer size: " << snd_rawmidi_params_get_buffer_size(params) << std::endl;
        snd_rawmidi_params_free(params);


        if (inputp)
                snd_rawmidi_read(input, NULL, 0); /* trigger reading */


        if (inputp) {
                int read = 0;
                int npfds, time = 0;
                struct pollfd *pfds;

                timeout *= 1000;
                npfds = snd_rawmidi_poll_descriptors_count(input);
                std::cout << "npfds: " << npfds << std::endl;
                pfds = (pollfd*)alloca(npfds * sizeof(struct pollfd));
                snd_rawmidi_poll_descriptors(input, pfds, npfds);
                unsigned char buf[1];
                //signal(SIGINT, sig_handler);
                for (;;) {
                        //unsigned char buf[256];
                        int i, length;
                        unsigned short revents;
                        err = poll(pfds, npfds, /* 1 */1000);
                        if (stop || (err < 0 && errno == EINTR))
                                break;
                        if (err < 0) {
                                error("poll failed: %s", strerror(errno));
                                break;
                        }
                        if (err == 0) {
                                time += 1;
                                if (timeout && time >= timeout)
                                        break;
                                continue;
                        }
                        if ((err = snd_rawmidi_poll_descriptors_revents(input, pfds, npfds, &revents)) < 0) {
                                error("cannot get poll events: %s", snd_strerror(errno));
                                break;
                        }
                        if (revents & (POLLERR | POLLHUP))
                                break;
                        if (!(revents & POLLIN))
                                continue;
                        err = snd_rawmidi_read(input, buf, 1);
                        if (err == -EAGAIN)
                                continue;
                        if (err < 0) {
                                error("cannot read from port \"%s\": %s", port_name, snd_strerror(err));
                                break;
                        }
                        length = 0;
                        for (i = 0; i < err; ++i) {
                                if (!ignore_active_sensing || buf[i] != 0xfe)  {
                                        buf[length++] = buf[i];
                                }
                        }
                        if (length == 0)
                                continue;
                        read += length;
                        time = 0;
                        for (i = 0; i < length; ++i)
                                synth->process_midimessage(buf[i]);
                        //fflush(stdout);
                }
        }

   return 0;
}


Synth::Synth(bool &stop) 
{
    // TODO: kick off thread that listens for input on stdin.
    pthread_create(&thread_id, NULL, synthThread, this);
}

Synth::~Synth()
{
}


float Synth::GetNextSample() {
    double t = (double) synthPos / GetSampleRate();
    short sample[4];
    for (int j=0; j<GetChannels(); j++) {
        double samp = sin((note_freq[midinotes[j]])*t*2*M_PI);
        sample[j] = midivolumes[j] * samp;
    }
    Sample s((uint8_t*)&sample, GetChannels(), GetBitsPerSample());
    synthPos++;
    return s.GetMonoValue();
}


std::vector<Sample> Synth::GetSamples(unsigned quantity, bool &stop) {

    unsigned int i = 0;
    std::vector<Sample> samples;

    while (i < quantity) {
      i++;

      double t = (double) synthPos / GetSampleRate();
      short sample[4];
      for (int j=0; j<GetChannels(); j++) {
          double samp = sin((note_freq[midinotes[j]])*t*2*M_PI);
          sample[j] = midivolumes[j] * samp;
      }
      samples.push_back(Sample((uint8_t*)&sample, GetChannels(), GetBitsPerSample()));

      synthPos++; // rolls over @ 65535
    }
    return samples;

}
