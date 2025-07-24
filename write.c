#include <stdio.h>
#include <gdal.h>
#include <ogr_srs_api.h>
#include "global.h"

#define INDEX(row, col) ((size_t)(row) * dir_map->ncols + (col))
#define DIR_NULL dir_map->null_value
#define DIR(row, col) dir_map->cells.byte[INDEX(row, col)]
#define IS_DIR_NULL(row, col) (DIR(row, col) == DIR_NULL)

int write_lfp(const char *output_path, const char *id_col,
              struct outlet_list *outlet_l, struct raster_map *dir_map,
              int append_layer)
{
    GDALDriverH driver = GDALGetDriverByName("GPKG");
    GDALDatasetH dataset;
    OGRSpatialReferenceH spat_ref;
    OGRLayerH layer;
    OGRFieldDefnH field_def;
    OGRFeatureDefnH layer_def;
    int lfp_id_field, length_field;
    int i;

    if (append_layer) {
        if (!(dataset =
              GDALOpenEx(output_path, GDAL_OF_VECTOR, NULL, NULL, NULL)))
            return 1;
    }
    else if (!
             (dataset =
              GDALCreate(driver, output_path, 0, 0, 0, GDT_Unknown, NULL)))
        return 1;

    spat_ref = OSRNewSpatialReference(dir_map->projection);
    layer =
        GDALDatasetCreateLayer(dataset, "lfp", spat_ref, wkbLineString, NULL);

    field_def = OGR_Fld_Create("lfp_id", OFTInteger);
    OGR_L_CreateField(layer, field_def, TRUE);
    OGR_Fld_Destroy(field_def);

    field_def = OGR_Fld_Create("length", OFTReal);
    OGR_L_CreateField(layer, field_def, TRUE);
    OGR_Fld_Destroy(field_def);

    layer_def = OGR_L_GetLayerDefn(layer);

    lfp_id_field = OGR_L_FindFieldIndex(layer, "lfp_id", TRUE);
    length_field = OGR_L_FindFieldIndex(layer, "length", TRUE);

    for (i = 0; i < outlet_l->n; i++) {
        int npnts = outlet_l->northo[i] + outlet_l->ndia[i] + 1;
        int j;

        for (j = 0; j < outlet_l->head_pl[i].n; j++) {
            int row = outlet_l->head_pl[i].row[j];
            int col = outlet_l->head_pl[i].col[j];
            double x, y;
            int pnt = 0;
            OGRFeatureH feature;
            OGRGeometryH geometry;

            feature = OGR_F_Create(layer_def);
            OGR_F_SetFieldInteger(feature, lfp_id_field, outlet_l->id[i]);
            OGR_F_SetFieldDouble(feature, length_field, outlet_l->lflen[i]);

            geometry = OGR_G_CreateGeometry(wkbLineString);
            OGR_G_SetPointCount(geometry, npnts);

            do {
                int dir;

                calc_coors(dir_map, row, col, &x, &y);
                OGR_G_SetPoint_2D(geometry, pnt++, x, y);

                switch ((dir = DIR(row, col))) {
                case NE:
                    row--;
                    col++;
                    break;
                case N:
                    row--;
                    break;
                case NW:
                    row--;
                    col--;
                    break;
                case W:
                    col--;
                    break;
                case SW:
                    row++;
                    col--;
                    break;
                case S:
                    row++;
                    break;
                case SE:
                    row++;
                    col++;
                    break;
                case E:
                    col++;
                    break;
                }
            } while (pnt < npnts);

            OGR_F_SetGeometry(feature, geometry);
            if (OGR_L_CreateFeature(layer, feature) != OGRERR_NONE)
                return 1;
            OGR_G_DestroyGeometry(geometry);
            OGR_F_Destroy(feature);
        }
    }

    GDALClose(dataset);
    OSRDestroySpatialReference(spat_ref);

    return 0;
}

int write_head_points(const char *output_path, const char *id_col,
                      struct outlet_list *outlet_l,
                      struct raster_map *dir_map, int append_layer)
{
    GDALDriverH driver = GDALGetDriverByName("GPKG");
    GDALDatasetH dataset;
    OGRSpatialReferenceH spat_ref;
    OGRLayerH layer;
    OGRFieldDefnH field_def;
    OGRFeatureDefnH layer_def;
    int lfp_id_field, x_field, y_field, length_field;
    int i;

    if (append_layer) {
        if (!(dataset =
              GDALOpenEx(output_path, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL,
                         NULL, NULL)))
            return 1;
    }
    else if (!
             (dataset =
              GDALCreate(driver, output_path, 0, 0, 0, GDT_Unknown, NULL)))
        return 1;

    spat_ref = OSRNewSpatialReference(dir_map->projection);
    layer =
        GDALDatasetCreateLayer(dataset, "lfp_heads", spat_ref, wkbPoint,
                               NULL);

    field_def = OGR_Fld_Create("lfp_id", OFTInteger);
    OGR_L_CreateField(layer, field_def, TRUE);
    OGR_Fld_Destroy(field_def);

    field_def = OGR_Fld_Create("x", OFTReal);
    OGR_L_CreateField(layer, field_def, TRUE);
    OGR_Fld_Destroy(field_def);

    field_def = OGR_Fld_Create("y", OFTReal);
    OGR_L_CreateField(layer, field_def, TRUE);
    OGR_Fld_Destroy(field_def);

    field_def = OGR_Fld_Create("length", OFTReal);
    OGR_L_CreateField(layer, field_def, TRUE);
    OGR_Fld_Destroy(field_def);

    layer_def = OGR_L_GetLayerDefn(layer);

    lfp_id_field = OGR_L_FindFieldIndex(layer, "lfp_id", TRUE);
    x_field = OGR_L_FindFieldIndex(layer, "x", TRUE);
    y_field = OGR_L_FindFieldIndex(layer, "y", TRUE);
    length_field = OGR_L_FindFieldIndex(layer, "length", TRUE);

    for (i = 0; i < outlet_l->n; i++) {
        int j;

        for (j = 0; j < outlet_l->head_pl[i].n; j++) {
            double x, y;
            OGRFeatureH feature;
            OGRGeometryH geometry;

            calc_coors(dir_map, outlet_l->head_pl[i].row[j],
                       outlet_l->head_pl[i].col[j], &x, &y);

            feature = OGR_F_Create(layer_def);
            OGR_F_SetFieldInteger(feature, lfp_id_field, outlet_l->id[i]);
            OGR_F_SetFieldDouble(feature, x_field, x);
            OGR_F_SetFieldDouble(feature, y_field, y);
            OGR_F_SetFieldDouble(feature, length_field, outlet_l->lflen[i]);

            geometry = OGR_G_CreateGeometry(wkbPoint);
            OGR_G_AddPoint_2D(geometry, x, y);
            OGR_F_SetGeometry(feature, geometry);
            if (OGR_L_CreateFeature(layer, feature) != OGRERR_NONE)
                return 1;
            OGR_G_DestroyGeometry(geometry);
            OGR_F_Destroy(feature);
        }
    }

    GDALClose(dataset);
    OSRDestroySpatialReference(spat_ref);

    return 0;
}

int write_head_coors(const char *heads_path, const char *id_col,
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
