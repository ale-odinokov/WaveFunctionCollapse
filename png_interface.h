#ifndef PNG_INTERAFCE_H
#define PNG_INTERAFCE_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include "datastruct.h"

struct pixel {
    int R;
    int G;
    int B;
    int alpha;
};    

// Read image from the PNG file.
int read_from_png(struct input_params *params, struct pixel **image);

// Write image to the PNG file. Alpha channel is omitted!
int write_to_png(struct input_params params, struct pixel *image);

// Calculate unique tiles and adjacency rules from the input image
int extract_tiles(struct input_params *params, struct pixel *image, int *ntiles,
                    struct pixel **tile_pixels, int **tile_counts, struct adjacency **arules);

// write current state to PNG file
int dump_state(char *fname, struct pixel *tile_pixels, int *tile_counts, int *mask,
                struct input_params params, int ntiles);

#ifdef __cplusplus
}
#endif

#endif /* PNG_INTERAFCE_H */

