#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdint.h>

#ifdef _MSC_VER
#include <winsock2.h>
/* gettimeofday.c */
int gettimeofday(struct timeval *, struct timezone *);
#else
#include <sys/time.h>
#endif

#define REALLOC_INCREMENT 1024

#define NE 128
#define N 64
#define NW 32
#define W 16
#define SW 8
#define S 4
#define SE 2
#define E 1

#define RASTER_MAP_TYPE_BYTE 0
#define RASTER_MAP_TYPE_UINT32 1
#define RASTER_MAP_TYPE_INT32 2

struct raster_map
{
    int type;
    int nrows, ncols;
    union
    {
        void *v;
        unsigned char *byte;
        unsigned int *uint32;
        int *int32;
    } cells;
    double null_value;
    char *projection;
    double geotransform[6];
    int compress;
};

struct outlet_list
{
    int nalloc, n;
    int *row, *col;
    int *id;
#ifndef USE_LESS_MEMORY
    char *has_up;
    int *down;
    double *flen;
#endif
    double *lflen;
    struct point_list *head_pl;
};

struct point_list
{
    int nalloc, n;
    int *row, *col;
};

#ifdef _MAIN_C_
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL int dir_checks[3][3]
#ifdef _MAIN_C_
    = {
    {SE, S, SW},
    {E, 0, W},
    {NE, N, NW}
}
#endif
;

/* timeval_diff.c */
long long timeval_diff(struct timeval *, struct timeval *, struct timeval *);

/* raster.c */
struct raster_map *init_raster(int, int, int);
void free_raster(struct raster_map *);
void copy_raster_metadata(struct raster_map *, const struct raster_map *);
struct raster_map *read_raster(const char *, int);
int write_raster(const char *, struct raster_map *);
void calc_row_col(struct raster_map *, double, double, int *, int *);
void calc_coors(struct raster_map *, int, int, double *, double *);

/* outlet_list.c */
void init_outlet_list(struct outlet_list *);
void reset_outlet_list(struct outlet_list *);
void free_outlet_list(struct outlet_list *);
void add_outlet(struct outlet_list *, int, int, int);

/* point_list.c */
void init_point_list(struct point_list *);
void reset_point_list(struct point_list *);
void free_point_list(struct point_list *);
void add_point(struct point_list *, int, int);

/* outlets.c */
struct outlet_list *read_outlets(char *, char *, struct raster_map *);
int write_outlets(const char *, struct outlet_list *);

/* melfp.c */
void melfp(struct raster_map *, struct outlet_list *, int, int);

/* lfp.c */
void lfp(struct raster_map *, struct outlet_list *, int);

/* lfp_lessmem.c */
void lfp_lessmem(struct raster_map *, struct outlet_list *, int);

/* heads.c */
int write_heads(const char *, const char *, struct outlet_list *,
                struct raster_map *);

#endif
