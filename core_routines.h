#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "datastruct.h"

int find_next_cell(int *sum_weights, double *sum_log_weights, double *random_entropy, int *cells, struct input_params *params);

int collapse_propagate( int cell_ndx,
                        int final_tile, // choose random tile if -1
                        int ntiles,
                        int *tile_counts,
                        struct adjacency *arules,
                        int **mask,
                        int **cells,
                        int **sum_weights,
                        double **sum_log_weights,
                        struct adjacency **ecount,
                        struct input_params *params);

// Partial collapse that excludes one tile. Useful for applying constraints
int exclude_propagate( int cell_ndx,
                        int final_tile, // choose random tile if -1
                        int ntiles,
                        int *tile_counts,
                        struct adjacency *arules,
                        int **mask,
                        int **cells,
                        int **sum_weights,
                        double **sum_log_weights,
                        struct adjacency **ecount,
                        struct input_params *params);


#ifdef __cplusplus
}
#endif

#endif /* CORE_H */

