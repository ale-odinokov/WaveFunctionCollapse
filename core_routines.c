#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "datastruct.h"

int find_next_cell(int *sum_weights, double *sum_log_weights, double *random_entropy, int *cells, struct input_params *params) {
    int i, imin;
    double w, w2, emin, e;

    emin = 1000000000000000.0;
    imin = -1;
    for (i = 0; i < params->out_width*params->out_height; i++) {
        if (cells[i] == -1) {
            w = (double)sum_weights[i];
            w2 = sum_log_weights[i];
            if (w > 0) {
                e = log(w) - w2/w + random_entropy[i];
            } else {
                e = random_entropy[i];
                printf("ERROR: failed to converge\n");
                return -1;
            }
//            printf("w = %8.4f,   w2 = %8.4f,   e = %8.4f\n", w, w2, e);
            if (e < emin) {
                imin = i;
                emin = e;
            }
        }
    }
    if (imin != -1) {
//        printf("entropy = %8.4f\n", emin);
        return imin;
    }
    else {
        printf("All cells collapsed!\n");
        return -2;
    }
}

int collapse_propagate( int cell_ndx,
                        int final_tile,
                        int ntiles,
                        int *tile_counts,
                        struct adjacency *arules,
                        int **mask,
                        int **cells,
                        int **sum_weights,
                        double **sum_log_weights,
                        struct adjacency **ecount,
                        struct input_params *params) {

    int i, i0, j, w, sum, ichosen;

    if (final_tile == -1) {
        w = rand() % (*sum_weights)[cell_ndx];
        sum = 0;
        for (i = 0; i < ntiles; i++) {
            if ((*mask)[cell_ndx*ntiles + i] == 1) {
                sum += tile_counts[i];
                if (sum >= w) {
                    ichosen = i;
                    break;
                }
            }
        }
        if (sum == 0) {
            return -1;
        }
    }
    else {
        if ((*mask)[cell_ndx*ntiles + final_tile] != 1) {
            return -1;
        }
        ichosen = final_tile;
    }
    (*cells)[cell_ndx] = ichosen;

    int ndelete = 0;
    // tiles to delete are stored as two integers: position and tile index
    int *todelete = (int *)malloc(sizeof(int)*params->out_width*params->out_height*ntiles*2);

    for (i = 0; i < ntiles; i++) {
        if (i != ichosen) {
            if ((*mask)[cell_ndx*ntiles + i] == 1) {
                todelete[2*ndelete] = cell_ndx;
                todelete[2*ndelete + 1] = i;
//                printf("deleting tile %d in cell %d\n", i, cell_ndx);
                (*mask)[cell_ndx*ntiles + i] = 0;
                (*sum_weights)[cell_ndx] -= tile_counts[i];
                (*sum_log_weights)[cell_ndx] -= tile_counts[i]*log(tile_counts[i]);
                ndelete++;
            }
        }
    }

    // main body of the propagation process
    int x0, y0; // position of the current tile
    int x, y; // position of the adjacent tile we want to check
    int tid; // tile index
    while (ndelete > 0) {
        i0 = todelete[2*ndelete - 2];
        y0 = i0 / params->out_width;
        x0 = i0 % params->out_width;
        tid = todelete[2*ndelete - 1];
        ndelete--;
        // check tile to the left
        y = y0;
        x = (params->out_width + x0 - 1) % params->out_width;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].left[j] == 1) {
                    (*ecount)[i].right[j]--;
                    if ((*ecount)[i].right[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
        // check tile to the right
        y = y0;
        x = (x0 + 1) % params->out_width;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].right[j] == 1) {
                    (*ecount)[i].left[j]--;
                    if ((*ecount)[i].left[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
        // check tile to the top
        y = (params->out_height + y0 - 1) % params->out_height;
        x = x0;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].top[j] == 1) {
                    (*ecount)[i].bottom[j]--;
                    if ((*ecount)[i].bottom[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
        // check tile to the bottom
        y = (y0 + 1) % params->out_height;
        x = x0;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].bottom[j] == 1) {
                    (*ecount)[i].top[j]--;
                    if ((*ecount)[i].top[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
    }

    free(todelete);
    return ichosen;
}

int exclude_propagate( int cell_ndx,
                        int final_tile,
                        int ntiles,
                        int *tile_counts,
                        struct adjacency *arules,
                        int **mask,
                        int **cells,
                        int **sum_weights,
                        double **sum_log_weights,
                        struct adjacency **ecount,
                        struct input_params *params) {

    int i, i0, j, w, sum, ichosen;

    if (final_tile == -1) {
        w = rand() % (*sum_weights)[cell_ndx];
        sum = 0;
        for (i = 0; i < ntiles; i++) {
            if ((*mask)[cell_ndx*ntiles + i] == 1) {
                sum += tile_counts[i];
                if (sum >= w) {
                    ichosen = i;
                    break;
                }
            }
        }
        if (sum == 0) {
            return -1;
        }
    }
    else {
        if ((*mask)[cell_ndx*ntiles + final_tile] != 1) {
            return -1;
        }
        ichosen = final_tile;
    }
    (*cells)[cell_ndx] = ichosen;

    int ndelete = 0;
    // tiles to delete are stored as two integers: position and tile index
    int *todelete = (int *)malloc(sizeof(int)*params->out_width*params->out_height*ntiles*2);

    for (i = 0; i < ntiles; i++) {
        if (i == ichosen) {
            if ((*mask)[cell_ndx*ntiles + i] == 1) {
                todelete[2*ndelete] = cell_ndx;
                todelete[2*ndelete + 1] = i;
//                printf("deleting tile %d in cell %d\n", i, cell_ndx);
                (*mask)[cell_ndx*ntiles + i] = 0;
                (*sum_weights)[cell_ndx] -= tile_counts[i];
                (*sum_log_weights)[cell_ndx] -= tile_counts[i]*log(tile_counts[i]);
                ndelete++;
            }
            break;
        }
    }

    // main body of the propagation process
    int x0, y0; // position of the current tile
    int x, y; // position of the adjacent tile we want to check
    int tid; // tile index
    while (ndelete > 0) {
        i0 = todelete[2*ndelete - 2];
        y0 = i0 / params->out_width;
        x0 = i0 % params->out_width;
        tid = todelete[2*ndelete - 1];
        ndelete--;
        // check tile to the left
        y = y0;
        x = (params->out_width + x0 - 1) % params->out_width;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].left[j] == 1) {
                    (*ecount)[i].right[j]--;
                    if ((*ecount)[i].right[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
        // check tile to the right
        y = y0;
        x = (x0 + 1) % params->out_width;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].right[j] == 1) {
                    (*ecount)[i].left[j]--;
                    if ((*ecount)[i].left[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
        // check tile to the top
        y = (params->out_height + y0 - 1) % params->out_height;
        x = x0;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].top[j] == 1) {
                    (*ecount)[i].bottom[j]--;
                    if ((*ecount)[i].bottom[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
        // check tile to the bottom
        y = (y0 + 1) % params->out_height;
        x = x0;
        i = y * params->out_width + x;
        for (j = 0; j < ntiles; j++) {
            if ((*mask)[i*ntiles + j] == 1) {
                if (arules[tid].bottom[j] == 1) {
                    (*ecount)[i].top[j]--;
                    if ((*ecount)[i].top[j] == 0) {
                        todelete[2*ndelete] = i;
                        todelete[2*ndelete + 1] = j;
//                        printf("deleting tile %d in cell %d\n", j, i);
                        (*mask)[i*ntiles + j] = 0;
                        (*sum_weights)[i] -= tile_counts[j];
                        (*sum_log_weights)[i] -= tile_counts[j]*log(tile_counts[j]);
                        ndelete++;
                    }
                }
            }
        }
    }

    free(todelete);
    return ichosen;
}