TARGET = robotfindskitten
APPNAME = "robotfindskitten"
APPID = "Rfk1"

OBJS = rfk-palm.o
IMAGES = $(TARGET).bmp $(TARGET)Small.bmp rfk.bmp
# (need -I../src so that we can use the same messages.h)
INCS = -I. -I../src

CC = m68k-palmos-gcc

#CFLAGS = -Wall -g -O2 $(INCS) -palmos2 -DI_AM_OS_2

# to compile with (HIGHLY EXPERIMENTAL) color:
#CFLAGS = -Wall -g -O2 $(INCS) -palmos3.5 -DNON_INTERNATIONAL -DI_AM_COLOR
## (if 3.5 is your default sdk you can just do:)
CFLAGS = -Wall -g -O2 $(INCS) -DNON_INTERNATIONAL -DI_AM_COLOR

PILRC = pilrc
BUILDPRC = build-prc

all: $(TARGET).prc

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

$(TARGET).prc: $(TARGET) bin.stamp
	$(BUILDPRC) $(TARGET).def $(TARGET) *.bin

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

bin.stamp: $(TARGET).rcp $(TARGET).pbm $(IMAGES)
	$(PILRC) $(TARGET).rcp .
	touch bin.stamp


depend:
	makedepend -Y  *.c

dingy:
	-rm -f *.[oa] $(TARGET)

clean:
	-rm -f *.[oa] $(TARGET) *.bin bin.stamp *.grc core

veryclean: clean
	-rm -f $(TARGET).prc *.ld *.s
# DO NOT DELETE

rfk-palm.o: robotfindskittenRsc.h constants.h
