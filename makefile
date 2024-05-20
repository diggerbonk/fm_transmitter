EXECUTABLE = fm_transmitter
VERSION = 0.9.6
FLAGS = -Wall -O3 -std=c++11
TRANSMITTER = -fno-strict-aliasing -I/opt/vc/include
ifeq ($(GPIO21), 1)
	TRANSMITTER += -DGPIO21
endif

all: fm_transmitter.o mailbox.o sample.o synth.o wave_reader.o transmitter.o cprofiler.o statsnode.o
	g++ -L/opt/vc/lib -o $(EXECUTABLE) fm_transmitter.o mailbox.o sample.o synth.o wave_reader.o transmitter.o cprofiler.o statsnode.o -lm -lpthread -lbcm_host -lasound

mailbox.o: mailbox.cpp mailbox.hpp
	g++ $(FLAGS) -c mailbox.cpp

sample.o: sample.cpp sample.hpp
	g++ $(FLAGS) -c sample.cpp

wave_reader.o: wave_reader.cpp wave_reader.hpp
	g++ $(FLAGS) -c wave_reader.cpp

synth.o: synth.cpp synth.hpp
	g++ $(FLAGS) -c synth.cpp

cprofiler.o: cprofiler.cpp cprofiler.hpp
	g++ $(FLAGS) -c cprofiler.cpp

statsnode.o: statsnode.cpp statsnode.hpp
	g++ $(FLAGS) -c statsnode.cpp


transmitter.o: transmitter.cpp transmitter.hpp
	g++ $(FLAGS) $(TRANSMITTER) -c transmitter.cpp

fm_transmitter.o: fm_transmitter.cpp
	g++ $(FLAGS) -DVERSION=\"$(VERSION)\" -DEXECUTABLE=\"$(EXECUTABLE)\" -c fm_transmitter.cpp

clean:
	rm *.o
