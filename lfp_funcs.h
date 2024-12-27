#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "global.h"

#define USE_TASKING_FOR_LAST_OUTLET

#define INDEX(row, col) ((size_t)(row) * ncols + (col))
#define DIR(row, col) dir_map->cells.byte[INDEX(row, col)]
#define IS_DIR_NULL(row, col) (DIR(row, col) == dir_map->null_value)

#ifdef USE_LESS_MEMORY
#define LFP lfp_lessmem
#define SET_OUTLET(row, col) do { DIR(row, col) = 0; } while(0)
#define IS_OUTLET(row, col) !DIR(row, col)
#define SET_DONE(row, col) do { DIR(row, col) += 5; } while(0)
#define IS_RECOVERABLE(row, col) (!IS_DIR_NULL(row, col) && \
        DIR(row, col) & (DIR(row, col) - 1))
#define RECOVER(row, col) do { DIR(row, col) -= 5; } while(0)
static unsigned char *outlet_dirs;
#else
#define LFP lfp
#ifdef USE_BITS_FOR_MORE_MEMORY
#define SIZE ((((size_t)nrows * ncols) >> 2) + \
        ((((size_t)nrows * ncols) & 3) != 0))
#define DONE(row, col) done[INDEX(row, col) >> 2]
#define BIT0(row, col) (1 << ((INDEX(row, col) & 3) << 1))
#define BIT1(row, col) (BIT0(row, col) << 1)
#define SET_OUTLET(row, col) do { DONE(row, col) |= BIT1(row, col); } while(0)
#define IS_OUTLET(row, col) (DONE(row, col) & BIT1(row, col))
#define SET_DONE(row, col) do { DONE(row, col) |= BIT0(row, col); } while(0)
#define IS_NOTDONE(row, col) !(DONE(row, col) & \
        (BIT0(row, col) | BIT1(row, col)))
#else
#define SIZE ((size_t)nrows * ncols)
#define DONE(row, col) done[INDEX(row, col)]
#define SET_OUTLET(row, col) do { DONE(row, col) |= 1; } while(0)
#define IS_OUTLET(row, col) (DONE(row, col) & 1)
#define SET_DONE(row, col) do { DONE(row, col) |= 2; } while(0)
#define IS_NOTDONE(row, col) !(DONE(row, col) & 3)
#endif
static char *done;
#endif

#ifdef USE_TASKING_FOR_LAST_OUTLET
static int remaining_nodes;
#endif

/* Relative row, col, and direction to the center in order of
 * E S W N SE SW NW NE */
static int nbr_rcd[8][3] = {
    {0, 1, W},
    {1, 0, N},
    {0, -1, E},
    {-1, 0, S},
    {1, 1, NW},
    {1, -1, NE},
    {-1, -1, SE},
    {-1, 1, SW}
};

struct cell
{
    int row;
    int col;
    int northo;
    int ndia;
};

struct cell_stack
{
    struct cell *cells;
    int n;
    int nalloc;
};

static int nrows, ncols;

#ifdef USE_TASKING_FOR_LAST_OUTLET
static int num_tasks;

#define TRACE_UP_RETURN int
#else
#define TRACE_UP_RETURN void
#endif


static TRACE_UP_RETURN trace_up(struct raster_map *, int, int, int, int,
                                int *, int *, double *, struct point_list *,
                                struct cell_stack *);
static void find_full_lfp(struct raster_map *, struct outlet_list *);
static void init_up_stack(struct cell_stack *);
static void free_up_stack(struct cell_stack *);
static void push_up(struct cell_stack *, struct cell *);
static struct cell pop_up(struct cell_stack *);

void LFP(struct raster_map *dir_map, struct outlet_list *outlet_l,
         int find_full
#ifdef USE_LESS_MEMORY
         , int preserve_dir
#endif
    )
{
    int i;

    nrows = dir_map->nrows;
    ncols = dir_map->ncols;

#ifdef USE_LESS_MEMORY
    outlet_dirs = calloc(outlet_l->n, sizeof *outlet_dirs);
#else
    done = calloc(SIZE, sizeof *done);
#endif

#pragma omp parallel for schedule(dynamic)
    for (i = 0; i < outlet_l->n; i++) {
#ifdef USE_LESS_MEMORY
        outlet_dirs[i] = DIR(outlet_l->row[i], outlet_l->col[i]);
#endif
        SET_OUTLET(outlet_l->row[i], outlet_l->col[i]);
    }

#ifdef USE_TASKING_FOR_LAST_OUTLET
#pragma omp parallel
#pragma omp single
    num_tasks = omp_get_num_threads() - 1;
    remaining_nodes = outlet_l->n;
#endif

    /* loop through all outlets and delineate the subwatershed for each */
#pragma omp parallel for schedule(dynamic)
    for (i = 0; i < outlet_l->n; i++) {
        struct cell_stack *up_stack = malloc(sizeof *up_stack);

        init_up_stack(up_stack);

        /* trace up flow directions */
#ifdef USE_TASKING_FOR_LAST_OUTLET
        /* if this outlet is the last one, use tasking for better load
         * balancing */
        if (
#endif
               trace_up(dir_map, outlet_l->row[i], outlet_l->col[i], 0, 0,
                        &outlet_l->northo[i], &outlet_l->ndia[i],
                        &outlet_l->lflen[i], &outlet_l->head_pl[i], up_stack)
#ifdef USE_TASKING_FOR_LAST_OUTLET
            ) {
            do {
                remaining_nodes = up_stack->n;
                /* calculate upstream longest flow paths at branching cells,
                 * compare their lengths, and add them if necessary */
                while (up_stack->n) {
                    struct cell up;

                    up = pop_up(up_stack);

#pragma omp task shared(up_stack)
                    {
                        int row = up.row, col = up.col;
                        int down_northo = up.northo, down_ndia = up.ndia;
                        int northo, ndia;
                        double lflen = 0;
                        struct point_list head_pl;
                        struct cell_stack *task_up_stack =
                            malloc(sizeof *task_up_stack);

                        init_point_list(&head_pl);
                        init_up_stack(task_up_stack);

                        /* calculate upstream longest flow paths from a
                         * branching node */
                        if (trace_up
                            (dir_map, row, col, down_northo, down_ndia,
                             &northo, &ndia, &lflen, &head_pl,
                             task_up_stack)) {
                            free_up_stack(up_stack);
                            up_stack = task_up_stack;
                        }
                        else
                            free_up_stack(task_up_stack);

                        /* this block is critical to avoid race conditions when
                         * updating outlet_l->*[i] values */
#pragma omp critical
                        {
                            /* if its longest flow length is longer than or
                             * equal to the previously found one, add new
                             * longest flow paths */
                            if (lflen >= outlet_l->lflen[i]) {
                                int j;

                                /* if it is longer, reomve all previous longest
                                 * flow paths first */
                                if (lflen > outlet_l->lflen[i]) {
                                    outlet_l->northo[i] = northo;
                                    outlet_l->ndia[i] = ndia;
                                    outlet_l->lflen[i] = lflen;
                                    reset_point_list(&outlet_l->head_pl[i]);
                                }

                                /* add all new longest flow paths */
                                for (j = 0; j < head_pl.n; j++)
                                    add_point(&outlet_l->head_pl[i],
                                              head_pl.row[j], head_pl.col[j]);
                            }
                        }

                        free_point_list(&head_pl);
                    }
                }
#pragma omp taskwait
            } while (up_stack->n);
        }
#endif
        /* XXX: work around an indent bug
         * #else
         *     ;
         * #endif
         * doesn't work */
        ;

        free_up_stack(up_stack);
    }

#ifdef USE_LESS_MEMORY
    /* if 5 was added previously (multiple bits), recover directions */
    if (find_full || preserve_dir) {
        int r, c;

#pragma omp parallel for schedule(dynamic)
        for (r = 0; r < nrows; r++)
            for (c = 0; c < ncols; c++) {
                if (IS_RECOVERABLE(r, c))
                    RECOVER(r, c);
            }
    }
#endif

    if (find_full)
        find_full_lfp(dir_map, outlet_l);

#ifdef USE_LESS_MEMORY
    /* recover outlet directions */
    if (preserve_dir) {
#pragma omp parallel for schedule(dynamic)
        for (i = 0; i < outlet_l->n; i++)
            DIR(outlet_l->row[i], outlet_l->col[i]) = outlet_dirs[i];
    }
    free(outlet_dirs);
#else
    free(done);
#endif
}

static TRACE_UP_RETURN trace_up(struct raster_map *dir_map, int row, int col,
                                int down_northo, int down_ndia, int *northo,
                                int *ndia, double *lflen,
                                struct point_list *head_pl,
                                struct cell_stack *up_stack)
{
    int i;
    int nup = 0;
    int next_row = -1, next_col = -1;
    int ortho = 0, dia = 0;

    for (i = 0; i < 8; i++) {
        int nbr_row = row + nbr_rcd[i][0];
        int nbr_col = col + nbr_rcd[i][1];

        /* skip edge cells */
        if (nbr_row < 0 || nbr_row >= nrows || nbr_col < 0 ||
            nbr_col >= ncols)
            continue;

        /* if a neighbor cell flows into the current cell, trace up further */
        if (DIR(nbr_row, nbr_col) == nbr_rcd[i][2]
#ifndef USE_LESS_MEMORY
            && IS_NOTDONE(nbr_row, nbr_col)
#endif
            && ++nup == 1) {
            /* climb up only to this cell at this time */
            next_row = nbr_row;
            next_col = nbr_col;
            ortho = i < 4;
            dia = !ortho;
            SET_DONE(next_row, next_col);
        }
    }

    if (!nup) {
        /* reached a ridge cell; if there were any up cells to visit, let's go
         * back or simply complete tracing */
        struct cell up;
        double flen = down_northo + down_ndia * M_SQRT2;

        if (flen >= *lflen) {
            if (flen > *lflen) {
                *northo = down_northo;
                *ndia = down_ndia;
                *lflen = flen;
                reset_point_list(head_pl);
            }
            add_point(head_pl, row, col);
        }

        if (!up_stack->n) {
#ifdef USE_TASKING_FOR_LAST_OUTLET
#pragma omp atomic
            remaining_nodes--;
            return 0;
#else
            return;
#endif
        }

#ifdef USE_TASKING_FOR_LAST_OUTLET
        if (remaining_nodes == 1 && up_stack->n >= num_tasks)
            return 1;
#endif

        up = pop_up(up_stack);
        next_row = up.row;
        next_col = up.col;
        down_northo = up.northo;
        down_ndia = up.ndia;
    }
    else if (nup > 1) {
        /* if there are more up cells to visit, let's come back later */
        struct cell up;

        up.row = row;
        up.col = col;
        up.northo = down_northo;
        up.ndia = down_ndia;
        push_up(up_stack, &up);
    }

#ifdef USE_TASKING_FOR_LAST_OUTLET
    if (remaining_nodes == 1 && up_stack->n >= num_tasks - 1) {
        struct cell up;

        up.row = next_row;
        up.col = next_col;
        up.northo = down_northo + ortho;
        up.ndia = down_ndia + dia;
        push_up(up_stack, &up);

        return 1;
    }
#endif

    /* use gcc -O2 or -O3 flags for tail-call optimization
     * (-foptimize-sibling-calls) */
#ifdef USE_TASKING_FOR_LAST_OUTLET
    return
#endif
        trace_up(dir_map, next_row, next_col, down_northo + ortho,
                 down_ndia + dia, northo, ndia, lflen, head_pl, up_stack);
}

static void find_full_lfp(struct raster_map *dir_map,
                          struct outlet_list *outlet_l)
{
    int i, j;
    int found_down = 0;

    if (outlet_l->n == 1)
        return;

#pragma omp parallel for schedule(dynamic)
    for (i = 0; i < outlet_l->n; i++) {
        int r = outlet_l->row[i], c = outlet_l->col[i];
        unsigned char dir;
        int northo = 0, ndia = 0;

#ifdef USE_LESS_MEMORY
        dir = outlet_dirs[i];
#else
        dir = DIR(r, c);
#endif

        do {
            switch (dir) {
            case NE:
                r--;
                c++;
                ndia++;
                break;
            case N:
                r--;
                northo++;
                break;
            case NW:
                r--;
                c--;
                ndia++;
                break;
            case W:
                c--;
                northo++;
                break;
            case SW:
                r++;
                c--;
                ndia++;
                break;
            case S:
                r++;
                northo++;
                break;
            case SE:
                r++;
                c++;
                ndia++;
                break;
            case E:
                c++;
                northo++;
                break;
            }
            dir = DIR(r, c);
        } while (r >= 0 && r < nrows && c >= 0 && c < ncols &&
                 !IS_DIR_NULL(r, c) && !IS_OUTLET(r, c));

        outlet_l->flen[i] = northo + ndia * M_SQRT2;

        if (IS_OUTLET(r, c)) {
            for (j = 0; j < outlet_l->n; j++) {
                if (j != i && outlet_l->row[j] == r && outlet_l->col[j] == c) {
                    outlet_l->has_up[j] = 1;
                    outlet_l->down[i] = j;
                    if (!found_down)
                        found_down = 1;
                    break;
                }
            }
        }
    }

    if (!found_down)
        return;

    /* cannot be parallelized because multiple threads could try to update a
     * common downstream outlet's head_pl, possibly causing a race condition */
    for (i = 0; i < outlet_l->n; i++) {
        int idx = i, down_idx = outlet_l->down[i];

        if (!outlet_l->has_up[idx] && down_idx >= 0) {
            do {
                double lflen_new = outlet_l->lflen[idx] + outlet_l->flen[idx];

                if (lflen_new >= outlet_l->lflen[down_idx]) {
                    if (lflen_new > outlet_l->lflen[down_idx]) {
                        reset_point_list(&outlet_l->head_pl[down_idx]);
                        outlet_l->lflen[down_idx] = lflen_new;
                    }
                    for (j = 0; j < outlet_l->head_pl[idx].n; j++)
                        add_point(&outlet_l->head_pl[down_idx],
                                  outlet_l->head_pl[idx].row[j],
                                  outlet_l->head_pl[idx].col[j]);
                }
                /* avoid duplicate tracing */
                outlet_l->down[idx] = -1;
                idx = down_idx;
                down_idx = outlet_l->down[idx];
            } while (down_idx >= 0);
        }
    }
}

static void init_up_stack(struct cell_stack *up_stack)
{
    up_stack->nalloc = up_stack->n = 0;
    up_stack->cells = NULL;
}

static void free_up_stack(struct cell_stack *up_stack)
{
    if (up_stack->cells)
        free(up_stack->cells);
    init_up_stack(up_stack);
}

static void push_up(struct cell_stack *up_stack, struct cell *up)
{
    if (up_stack->n == up_stack->nalloc) {
        up_stack->nalloc += REALLOC_INCREMENT;
        up_stack->cells =
            realloc(up_stack->cells,
                    sizeof *up_stack->cells * up_stack->nalloc);
    }
    up_stack->cells[up_stack->n++] = *up;
}

static struct cell pop_up(struct cell_stack *up_stack)
{
    struct cell up;

    if (up_stack->n > 0) {
        up = up_stack->cells[--up_stack->n];
        if (up_stack->n == 0)
            free_up_stack(up_stack);
        else if (up_stack->n == up_stack->nalloc - REALLOC_INCREMENT) {
            up_stack->nalloc -= REALLOC_INCREMENT;
            up_stack->cells =
                realloc(up_stack->cells,
                        sizeof *up_stack->cells * up_stack->nalloc);
        }
    }

    return up;
}
