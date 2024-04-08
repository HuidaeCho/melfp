#include <stdio.h>
#include "global.h"

int write_heads(const char *heads_path, const char *id_col,
                struct outlet_list *outlet_l, struct raster_map *dir_map)
{
    FILE *fp;
    int i;

    if (!(fp = fopen(heads_path, "w")))
        return 1;

    fprintf(fp, "%s,lfp_id,x,y,row,column,length\n", id_col);

    for (i = 0; i < outlet_l->n; i++) {
        int j;

        for (j = 0; j < outlet_l->head_pl[i].n; j++) {
            int row = outlet_l->head_pl[i].row[j];
            int col = outlet_l->head_pl[i].col[j];
            double x, y;

            calc_coors(dir_map, row, col, &x, &y);
            fprintf(fp, "%d,%d,%f,%f,%d,%d,%f\n", outlet_l->id[i], j + 1, x,
                    y, row, col, outlet_l->lflen[i]);
        }
    }

    fclose(fp);

    return 0;
}
