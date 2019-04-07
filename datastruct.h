#ifndef DATASTRUCT_H
#define DATASTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TILES 4096 // maximum number of tiles, including non-unique

// Definition of the input parameters
struct input_params {
    char *inp_filename;
    char *out_filename;
    int tile_size;
    int width;
    int height;
    int rot_aug; // rotational augmentation of the input image
    int h_aug;   // horizontal reflection of the input image
    int v_aug;   // vertical reflection of the input image
    int out_width;
    int out_height;
    int constraints; // use template for constraints
    char *constraints_filename;
};

// Adjacency rules for the tile.
struct adjacency {
    int left[MAX_TILES];
    int right[MAX_TILES];
    int top[MAX_TILES];
    int bottom[MAX_TILES];
};

#ifdef __cplusplus
}
#endif

#endif /* DATASTRUCT_H */

