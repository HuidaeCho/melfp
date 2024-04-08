CFLAGS=-Wall -Werror -O3 -fopenmp
LDFLAGS=-O3 -fopenmp

all: melfp

clean:
	$(RM) *.o

melfp: \
	main.o \
	timeval_diff.o \
	raster.o \
	outlet_list.o \
	point_list.o \
	outlets.o \
	melfp.o \
	lfp.o \
	lfp_lessmem.o \
	heads.o
	$(CC) $(LDFLAGS) -o $@ $^ `gdal-config --libs`

*.o: global.h
lfp*.o: lfp_funcs.h
