TARGET = robotfindskitten
APPNAME = "robotfindskitten"
APPID = "Rfk1"

OBJS = rfk-palm.o
IMGS = $(TARGET).bmp $(TARGET)Small.bmp rfk.bmp

CC = m68k-palmos-gcc

# (need -I../src so that we can use the same messages.h)
CFLAGS = -Wall -g -O2 -I. -I../src -palmos2 -DI_AM_OS_2

# to compile with (HIGHLY EXPERIMENTAL) color:
#CFLAGS = -Wall -g -O2 -I. -I../src -palmos3.5 -DNON_INTERNATIONAL -DI_AM_COLOR
## (if 3.5 is your default sdk you can just do:)
##CFLAGS = -Wall -g -O2 -I. -I../src -DNON_INTERNATIONAL -DI_AM_COLOR

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

bin.stamp: $(TARGET).rcp $(TARGET).pbm $(IMGS)
	$(PILRC) $(TARGET).rcp .
	touch bin.stamp

#$(TARGET)-sections.o: $(TARGET)-sections.s
#	$(CC) -c $(TARGET)-sections.s

#$(TARGET)-sections.s $(TARGET)-sections.ld: $(TARGET).def
#	multigen $(TARGET).def

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
