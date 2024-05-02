#define _MAIN_C_

#include <stdlib.h>
#include <string.h>
#include <gdal.h>
#include <cpl_conv.h>
#include <omp.h>
#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <sys/time.h>
#endif
#include "global.h"

int main(int argc, char *argv[])
{
    int i;
    int print_usage = 1, write_outlet = 0, find_full = 0, use_lessmem = 0;
    char *dir_path = NULL, *outlets_path = NULL, *id_col =
        NULL, *output_path = NULL;
    struct raster_map *dir_map;
    struct outlet_list *outlet_l;
    struct timeval start_time, end_time;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            int j, n = strlen(argv[i]);
            int unknown = 0;

            for (j = 1; j < n && !unknown; j++) {
                switch (argv[i][j]) {
                case 'o':
                    write_outlet = 1;
                    break;
                case 'f':
                    find_full = 1;
                    break;
                case 'l':
                    use_lessmem = 1;
                    break;
                default:
                    unknown = 1;
                    break;
                }
            }
            if (unknown) {
                fprintf(stderr, "%c: Unknown flag\n", argv[i][j]);
                print_usage = 2;
                break;
            }
        }
        else if (!dir_path)
            dir_path = argv[i];
        else if (!outlets_path)
            outlets_path = argv[i];
        else if (!id_col)
            id_col = argv[i];
        else if (!output_path) {
            output_path = argv[i];
            print_usage = 0;
        }
        else {
            fprintf(stderr, "%s: Unable to process extra arguments\n",
                    argv[i]);
            print_usage = 2;
            break;
        }
    }

    if (print_usage) {
        if (print_usage == 2)
            printf("\n");
        printf("Usage: melfp [-ofl] fdr.tif outlets.shp id_col output.ext\n");
        printf("\n");
        printf("  -o\t\tWrite outlet rows and columns, and exit\n");
        printf("  -f\t\tFind full longest flow paths\n");
        printf("  -l\t\tUse less memory (cannot be used with -f)\n");
        printf("  fdr.tif\tInput flow direction GeoTIFF\n");
        printf("  outlets.shp\tInput outlets Shapefile\n");
        printf("  id_col\tID column\n");
        printf
            ("  output.ext\tOutput longest flow path head coordinatess (TODO: Shapefile)\n");
        printf
            ("  \t\tOutput text file for outlet rows and columns with -o\n");
        exit(print_usage == 1 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    if (find_full && use_lessmem) {
        fprintf(stderr,
                "Cannot calculate full longest flow paths with less memory\n");
        exit(EXIT_FAILURE);
    }

    GDALAllRegister();

    printf("Reading flow direction raster <%s>...\n", dir_path);
    gettimeofday(&start_time, NULL);
    if (!(dir_map = read_raster(dir_path, RASTER_MAP_TYPE_BYTE))) {
        fprintf(stderr, "%s: Failed to read flow direction raster\n",
                dir_path);
        exit(EXIT_FAILURE);
    }
    gettimeofday(&end_time, NULL);
    printf("Input time for flow direction: %lld microsec\n",
           timeval_diff(NULL, &end_time, &start_time));

    printf("Reading outlets <%s>...\n", outlets_path);
    gettimeofday(&start_time, NULL);
    if (!(outlet_l = read_outlets(outlets_path, id_col, dir_map))) {
        fprintf(stderr, "%s: Failed to read outlets\n", outlets_path);
        exit(EXIT_FAILURE);
    }
    gettimeofday(&end_time, NULL);
    printf("Input time for outlets: %lld microsec\n",
           timeval_diff(NULL, &end_time, &start_time));

    if (write_outlet) {
        printf("Writing outlets...\n");
        gettimeofday(&start_time, NULL);
        if (write_outlets(output_path, outlet_l) > 0) {
            fprintf(stderr, "%s: Failed to write outlets file\n",
                    output_path);
            free_raster(dir_map);
            free_outlet_list(outlet_l);
            exit(EXIT_FAILURE);
        }
        gettimeofday(&end_time, NULL);
        printf("Output time for outlets: %lld microsec\n",
               timeval_diff(NULL, &end_time, &start_time));
    }
    else {
#pragma omp parallel
#pragma omp single
        printf("Finding longest flow paths using %d thread(s)...\n",
               omp_get_num_threads());
        gettimeofday(&start_time, NULL);
        melfp(dir_map, outlet_l, find_full, use_lessmem);
        gettimeofday(&end_time, NULL);
        printf
            ("Computation time for longest flow paths: %lld microsec\n",
             timeval_diff(NULL, &end_time, &start_time));

        printf("Writing longest flow path source points <%s>...\n",
               output_path);
        gettimeofday(&start_time, NULL);
        if (write_heads(output_path, id_col, outlet_l, dir_map) > 0) {
            fprintf(stderr,
                    "%s: Failed to write longest flow path source points\n",
                    output_path);
            free_raster(dir_map);
            free_outlet_list(outlet_l);
            exit(EXIT_FAILURE);
        }
        gettimeofday(&end_time, NULL);
        printf
            ("Output time for longest flow path source points: %lld microsec\n",
             timeval_diff(NULL, &end_time, &start_time));
    }

    free_raster(dir_map);
    free_outlet_list(outlet_l);

    exit(EXIT_SUCCESS);
}
