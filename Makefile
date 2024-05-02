ifeq ($(OS),Windows_NT)
	CFLAGS=-Wall -Werror -O3 -fopenmp -I/c/OSGeo4W/include
	GDALLIBS=/c/OSGeo4W/lib/gdal_i.lib
	EXT=.exe
else
	CFLAGS=-Wall -Werror -O3 -fopenmp
	GDALLIBS=`gdal-config --libs`
	EXT=
endif
LDFLAGS=-O3 -fopenmp

all: melfp$(EXT)

clean:
	$(RM) *.o

melfp$(EXT): \
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
	$(CC) $(LDFLAGS) -o $@ $^ $(GDALLIBS)

*.o: global.h raster.h
lfp*.o: lfp_funcs.h
