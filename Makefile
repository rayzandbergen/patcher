raspbian=

all: patcher tags

CXX=g++
CC=invalid

CXXFLAGS=-Wall -Wextra -O3
#CXXFLAGS=-Wall -Wextra -g
LDFLAGS=-lasound -lrt

ifdef raspbian
CXXFLAGS+=-DRASPBIAN
LDFLAGS+=-lncurses
else
LDFLAGS+=-lncursesw
endif

CFLAGS=$(CXXFLAGS)
HEADERS=midi.h screen.h dump.h trackdef.h fantom.h now.h monofilter.h transposer.h timestamp.h activity.h patcher.h midi_note.h controller.h toggler.h networkif.h sequencer.h alarm.h
now.h:
	echo "#define NOW \""`date '+%d %b %Y'`"\"" > $@
	echo "#define SVN \""`svnversion -n`"\"" >> $@
tags: patcher
	ctags -R
fantom.o: fantom.cpp $(HEADERS)
patcher.o: patcher.cpp $(HEADERS)
screen.o: screen.cpp $(HEADERS)
midi.o: midi.cpp $(HEADERS)
dump.o: dump.cpp $(HEADERS)
trackdef.o: trackdef.cpp $(HEADERS)
monofilter.o: monofilter.cpp $(HEADERS)
transposer.o: transposer.cpp $(HEADERS)
timestamp.o: timestamp.cpp $(HEADERS)
activity.o: activity.cpp $(HEADERS)
controller.o: controller.cpp $(HEADERS)
toggler.o: toggler.cpp $(HEADERS)
networkif.o: networkif.cpp $(HEADERS)
sequencer.o: sequencer.cpp $(HEADERS)
alarm.o: alarm.cpp $(HEADERS)
patcher: fantom.o patcher.o screen.o midi.o dump.o trackdef.o monofilter.o transposer.o timestamp.o activity.o controller.o toggler.o networkif.o sequencer.o alarm.o
	$(CXX) $(LDFLAGS) -o $@ $^
distclean:
	-rm -rf *.o defs.h defs.cpp patcher fantom_cache.bin tags
clean: distclean
	-rm -rf now.h
