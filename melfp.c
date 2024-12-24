#include "global.h"

void melfp(struct raster_map *dir_map, struct outlet_list *outlet_l,
           int find_full, int use_lessmem)
{
    switch (use_lessmem) {
    case 0:
        lfp(dir_map, outlet_l, find_full);
        break;
    case 1:
        lfp_lessmem(dir_map, outlet_l, find_full, 0);
        break;
    case 2:
        lfp_lessmem(dir_map, outlet_l, find_full, 1);
        break;
    }
}
