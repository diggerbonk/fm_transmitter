/*
 *  amidi.c - read from/write to RawMIDI ports
 *
 *  Copyright (c) Clemens Ladisch <clemens@ladisch.de>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>

static const char *port_name = "hw:1,0,0";
static int ignore_active_sensing = 1;
static int timeout;
static int stop;
static snd_rawmidi_t *input, **inputp;
static unsigned char midinote;
static unsigned char midicommand;

const uint16_t note_freq[] = {
      0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, /*   0 -  11 */
     16,   17,   18,   19,   21,   22,   23,   25,   26,   28,   29,   31, /*  12 -  23 */
     33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62, /*  24 -  35 */
     65,   69,   73,   78,   82,   87,   93,   98,  104,  110,  117,  123, /*  36 -  47 */
    131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247, /*  48 -  59 */
    262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494, /*  60 -  71 */
    523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988, /*  72 -  83 */
   1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, /*  84 -  95 */
   2093, 2218, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, /*  96 - 107 */
   4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902, /* 108 - 119 */
   8372, 8870, 9397, 9956,10548,11175,11840,12544                          /* 120 - 127 */
};



static void error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	putc('\n', stderr);
}


static void process_midimessage(unsigned char byte)
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
	int newline = 0;

	if (byte >= 0xf8)
		newline = 1;
	else if (byte >= 0xf0) {
		newline = 1;
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
			newline = state != STATE_SYSEX;
			state = STATE_UNKNOWN;
			break;
		}
	} else if (byte >= 0x80) {
		newline = 1;
		if (byte >= 0xc0 && byte <= 0xdf) {
			state = STATE_1PARAM;
		}
		else  {
			state = STATE_2PARAM_1;
		        midicommand = byte;
		}
	} else /* b < 0x80 */ {
		int running_status = 0;
		newline = state == STATE_UNKNOWN;
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

	uint16_t notefreq = note_freq[(int)midinote];
	if (state == 5) {
		//printf("midicommand %02X, frequency %i\n", midicommand, notefreq);
		printf("%i\n", notefreq);
	}
}

static void sig_handler(int dummy)
{
	stop = 1;
}

int main(int argc, char *argv[])
{
    int c, err, ok = 0;

    inputp = &input;

	if ((err = snd_rawmidi_open(inputp, NULL, port_name, SND_RAWMIDI_NONBLOCK)) < 0) {
		error("cannot open port \"%s\": %s", port_name, snd_strerror(err));
		goto _exit2;
	}

	if (inputp)
		snd_rawmidi_read(input, NULL, 0); /* trigger reading */


	if (inputp) {
		int read = 0;
		int npfds, time = 0;
		struct pollfd *pfds;

		timeout *= 1000;
		npfds = snd_rawmidi_poll_descriptors_count(input);
		pfds = alloca(npfds * sizeof(struct pollfd));
		snd_rawmidi_poll_descriptors(input, pfds, npfds);
		signal(SIGINT, sig_handler);
		for (;;) {
			unsigned char buf[256];
			int i, length;
			unsigned short revents;

			err = poll(pfds, npfds, 200);
			if (stop || (err < 0 && errno == EINTR))
				break;
			if (err < 0) {
				error("poll failed: %s", strerror(errno));
				break;
			}
			if (err == 0) {
				time += 200;
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
			err = snd_rawmidi_read(input, buf, sizeof(buf));
			if (err == -EAGAIN)
				continue;
			if (err < 0) {
				error("cannot read from port \"%s\": %s", port_name, snd_strerror(err));
				break;
			}
			length = 0;
			for (i = 0; i < err; ++i)
				if (!ignore_active_sensing || buf[i] != 0xfe)
					buf[length++] = buf[i];
			if (length == 0)
				continue;
			read += length;
			time = 0;
			for (i = 0; i < length; ++i)
				process_midimessage(buf[i]);
			fflush(stdout);
		}
	}

	ok = 1;
_exit:
	if (inputp)
		snd_rawmidi_close(input);
_exit2:
	return !ok;
}
