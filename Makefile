ifeq ($(OS),Windows_NT)
	GDAL_CFLAGS=-I/c/OSGeo4W/include
	GDAL_LIBS=/c/OSGeo4W/lib/gdal_i.lib
	EXT=.exe
else
	GDAL_LIBS=`gdal-config --libs`
endif
CFLAGS=-Wall -Werror -O3 -fopenmp $(GDAL_CFLAGS)
LDFLAGS=-fopenmp -lm

all: melfp$(EXT)

clean:
	$(RM) *.o

melfp$(EXT): \
	main.o \
	timeval_diff.o \
	raster.o \
	recode.o \
	outlet_list.o \
	point_list.o \
	outlets.o \
	lfp.o \
	lfp_lessmem.o \
	lfp_moremem.o \
	heads.o
	$(CC) $(LDFLAGS) -o $@ $^ $(GDAL_LIBS)

*.o: global.h raster.h
lfp*.o: lfp_funcs.h
