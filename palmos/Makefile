TARGET = robotfindskitten
APPNAME = "robotfindskitten"
APPID = "Rfk1"

OBJS = rfk-palm.o rfk-memo.o
IMAGES = $(TARGET).bmp $(TARGET)Small.bmp rfk.bmp
# (need -I../src so that we can use the same messages.h)
INCS = -I. -I../src

CC = m68k-palmos-gcc
PERL = perl


# well, it's not really necessary to compile conditionally.  the color rfk
# checks your OS version so it's quite possible to run it on older OS.
# ifdeffing out the color code saves us a whopping.. let's see.. 716 bytes
# in version 003.  So I think I'll just compile one version from now on.
#CFLAGS = -Wall -g -O2 $(INCS) -palmos2 -DI_AM_OS_2

# to compile with (HIGHLY EXPERIMENTAL) color:
#CFLAGS = -Wall -g -O2 $(INCS) -palmos3.5 -DNON_INTERNATIONAL -DI_AM_COLOR
# if 3.5 is your default sdk you can just do this:
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

# Check it out.. automatic subversion-number updating via cheesy perl script
$(TARGET).rcp: $(TARGET).rcp.in ../src/messages.h
	perl -n preprocess_rcp.pl < $(TARGET).rcp.in > $(TARGET).rcp

depend:
	makedepend -Y  *.c

dingy:
	-rm -f *.[oa] $(TARGET)

clean:
	-rm -f *.[oa] $(TARGET) *.bin bin.stamp *.grc core $(TARGET).rcp

veryclean: clean
	-rm -f $(TARGET).prc *.ld *.s
# DO NOT DELETE

rfk-memo.o: constants.h ../src/messages.h
rfk-palm.o: robotfindskittenRsc.h constants.h rfk-memo.h ../src/messages.h
