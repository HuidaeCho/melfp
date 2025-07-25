#define _MAIN_C_

#include <stdlib.h>
#include <string.h>
#include <gdal.h>
#include <cpl_conv.h>
#include <math.h>
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
    int print_usage = 1, write_outlet = 0, find_full = 0, use_lessmem =
        2, output = 1;
    double (*recode)(double, void *) = NULL;
    int *recode_data = NULL, encoding[8];
    char *dir_path = NULL, *outlets_path = NULL, *id_col =
        NULL, *output_path = NULL, *coors_path = NULL;
    struct raster_map *dir_map;
    struct outlet_list *outlet_l;
    struct timeval start_time, end_time;

#ifdef LOOP_THEN_TASK
    char *p;

    if ((p = getenv("MELFP_TRACING_STACK_SIZE")))
        tracing_stack_size = atoi(p);
    else
        tracing_stack_size = 1024 * 3;
#endif

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
                case 'm':
                    use_lessmem = 0;
                    break;
                case 'P':
                    if (use_lessmem)
                        use_lessmem = 1;
                    break;
                case 'e':
                    if (i == argc - 1) {
                        fprintf(stderr, "-%c: Missing encoding\n",
                                argv[i][j]);
                        print_usage = 2;
                        break;
                    }
                    if (strcmp(argv[++i], "power2") == 0) {
                        recode = NULL;
                        recode_data = NULL;
                        break;
                    }
                    else if (strcmp(argv[i], "taudem") == 0) {
                        int k;

                        for (k = 1; k < 9; k++)
                            encoding[k % 8] = 9 - k;
                    }
                    else if (strcmp(argv[i], "45degree") == 0) {
                        int k;

                        for (k = 0; k < 8; k++)
                            encoding[k] = 8 - k;
                    }
                    else if (strcmp(argv[i], "degree") == 0) {
                        recode = recode_degree;
                        recode_data = NULL;
                        break;
                    }
                    else if (sscanf
                             (argv[i], "%d,%d,%d,%d,%d,%d,%d,%d",
                              &encoding[0], &encoding[1], &encoding[2],
                              &encoding[3], &encoding[4], &encoding[5],
                              &encoding[6], &encoding[7]) != 8) {
                        fprintf(stderr, "%s: Invalid encoding\n", argv[i]);
                        print_usage = 2;
                        break;
                    }
                    recode = recode_encoding;
                    recode_data = encoding;
                    break;
#ifdef LOOP_THEN_TASK
                case 's':
                    if (i == argc - 1) {
                        fprintf(stderr, "-%c: Missing tracing stack size\n",
                                argv[i][j]);
                        print_usage = 2;
                        break;
                    }
                    tracing_stack_size = atoi(argv[++i]);
                    break;
#endif
                case 'L':
                    output &= ~1;
                    break;
                case 'p':
                    output |= 2;
                    break;
                case 'c':
                    output |= 4;
                    if (i == argc - 1) {
                        fprintf(stderr,
                                "-%c: Missing output coordinates path\n",
                                argv[i][j]);
                        print_usage = 2;
                        break;
                    }
                    coors_path = argv[++i];
                    break;
                default:
                    unknown = 1;
                    break;
                }
            }
            if (unknown) {
                fprintf(stderr, "-%c: Unknown flag\n", argv[i][j]);
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

    if (!output) {
        fprintf(stderr, "Missing output formats\n");
        print_usage = 2;
    }
    else if (output & 1 && use_lessmem == 1) {
        fprintf(stderr,
                "Forced to preserve input data for vector routing; Ignoring -P\n");
        use_lessmem = 2;
    }

    if (print_usage) {
        if (print_usage == 2)
            printf("\n");
        printf
            ("Usage: melfp OPTIONS fdr.tif outlets.shp id_col output.gpkg\n");
        printf("\n");
        printf("  -o\t\tWrite outlet rows and columns, and exit\n");
        printf("  -f\t\tFind full longest flow paths\n");
        printf("  -m\t\tUse more memory\n");
        printf("  -P\t\tDo not preserve input data (faster with -L)\n");
        printf("  -e encoding\tDirection encoding\n");
        printf
            ("\t\tpower2 (default): 2^0-7 CW from E (e.g., r.terraflow, ArcGIS)\n");
        printf("\t\ttaudem: 1-8 (E-SE CCW) (e.g., d8flowdir)\n");
        printf("\t\t45degree: 1-8 (NE-E CCW) (e.g., r.watershed)\n");
        printf("\t\tdegree: (0,360] (E-E CCW)\n");
        printf
            ("\t\tE,SE,S,SW,W,NW,N,NE: custom (e.g., 1,8,7,6,5,4,3,2 for taudem)\n");
        printf("  -L\t\tDo not output longest flow path lines\n");
        printf("  -p\t\tOutput longest flow path headwater points\n");
        printf
            ("  -c coors.csv\tOutput longest flow path headwater coordinates\n");
#ifdef LOOP_THEN_TASK
        printf("  -s size\tTracing stack size (default %d)\n",
               tracing_stack_size);
#endif
        printf("  fdr.tif\tInput flow direction GeoTIFF\n");
        printf("  outlets.shp\tInput outlets Shapefile\n");
        printf("  id_col\tID column\n");
        printf
            ("  output.gpkg\tOutput longest flow path GeoPackage\n"
             "  \t\tOutput text file for outlet rows and columns with -o\n");
        exit(print_usage == 1 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    GDALAllRegister();

    printf("Reading flow direction raster <%s>...\n", dir_path);
    gettimeofday(&start_time, NULL);
    if (recode) {
        printf("Converting flow direction encoding...\n");
        if (!(dir_map = read_raster(dir_path, RASTER_MAP_TYPE_BYTE, 0, recode,
                                    recode_data))) {
            fprintf(stderr, "%s: Failed to read flow direction raster\n",
                    dir_path);
            exit(EXIT_FAILURE);
        }
    }
    else if (!(dir_map =
               read_raster(dir_path, RASTER_MAP_TYPE_BYTE, 0, NULL, NULL))) {
        fprintf(stderr, "%s: Failed to read flow direction raster\n",
                dir_path);
        exit(EXIT_FAILURE);
    }
    gettimeofday(&end_time, NULL);
    printf("Input time for flow direction: %lld microsec\n",
           timeval_diff(NULL, &end_time, &start_time));

    printf("Reading outlets <%s>...\n", outlets_path);
    gettimeofday(&start_time, NULL);
    if (!(outlet_l = read_outlets(outlets_path, id_col, dir_map, find_full))) {
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
        int append_layer = 0;
        int num_lfp = 0;

#pragma omp parallel
#pragma omp single
        {
            size_t num_cells = (size_t)dir_map->nrows * dir_map->ncols;
            int num_threads = omp_get_num_threads();

            printf("Number of cells: %zu\n", num_cells);
            printf("Number of outlets: %d\n", outlet_l->n);

#ifdef LOOP_THEN_TASK
            if (tracing_stack_size <= 0) {
                printf
                    ("Guessing tracing stack size using sqrt(nrows * ncols) / num_threads...\n");
                tracing_stack_size =
                    sqrt((size_t)dir_map->nrows * dir_map->ncols) /
                    num_threads;
            }

            printf("Tracing stack size for loop-then-task: %d\n",
                   tracing_stack_size);
#endif
            printf("Finding longest flow paths using %d thread(s)...\n",
                   num_threads);
        }

        gettimeofday(&start_time, NULL);
        lfp(dir_map, outlet_l, find_full, use_lessmem);
        gettimeofday(&end_time, NULL);
        printf
            ("Computation time for longest flow paths: %lld microsec\n",
             timeval_diff(NULL, &end_time, &start_time));

        for (i = 0; i < outlet_l->n; i++)
            num_lfp += outlet_l->head_pl[i].n;
        printf("Number of longest flow paths found: %d\n", num_lfp);

        if (output & 1) {
            printf("Writing longest flow path lines <%s>...\n", output_path);
            gettimeofday(&start_time, NULL);
            if (write_lfp
                (output_path, id_col, outlet_l, dir_map, append_layer) > 0) {
                fprintf(stderr,
                        "%s: Failed to write longest flow path lines\n",
                        output_path);
                free_raster(dir_map);
                free_outlet_list(outlet_l);
                exit(EXIT_FAILURE);
            }
            gettimeofday(&end_time, NULL);
            printf("Output time for longest flow path lines: %lld microsec\n",
                   timeval_diff(NULL, &end_time, &start_time));
            append_layer = 1;
        }

        if (output & 2) {
            printf("Writing longest flow path headwater points <%s>...\n",
                   output_path);
            gettimeofday(&start_time, NULL);
            if (write_head_points
                (output_path, id_col, outlet_l, dir_map, append_layer) > 0) {
                fprintf(stderr,
                        "%s: Failed to write longest flow path headwater points\n",
                        output_path);
                free_raster(dir_map);
                free_outlet_list(outlet_l);
                exit(EXIT_FAILURE);
            }
            gettimeofday(&end_time, NULL);
            printf
                ("Output time for longest flow path headwater points: %lld microsec\n",
                 timeval_diff(NULL, &end_time, &start_time));
            append_layer = 1;
        }

        if (output & 4) {
            printf
                ("Writing longest flow path headwater coordinates <%s>...\n",
                 coors_path);
            gettimeofday(&start_time, NULL);
            if (write_head_coors(coors_path, id_col, outlet_l, dir_map) > 0) {
                fprintf(stderr,
                        "%s: Failed to write longest flow path headwater coordinates\n",
                        coors_path);
                free_raster(dir_map);
                free_outlet_list(outlet_l);
                exit(EXIT_FAILURE);
            }
            gettimeofday(&end_time, NULL);
            printf
                ("Output time for longest flow path headwater coordinates: %lld microsec\n",
                 timeval_diff(NULL, &end_time, &start_time));
        }
    }

    free_raster(dir_map);
    free_outlet_list(outlet_l);

    exit(EXIT_SUCCESS);
}
