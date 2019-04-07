#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "datastruct.h"
#include "png_interface.h"
#include "core_routines.h"

int generate_image(struct input_params *params) {
    int i, j, k;
    time_t t;

    srand((unsigned) time(&t));

    struct pixel *inp_image;
    read_from_png(params, &inp_image);

    int ntiles;
    struct pixel *tile_pixels;
    struct adjacency *arules;
    int *tile_counts;

    printf("Generating tiles from the input image %s\n", params->inp_filename);
    int res1 = extract_tiles(params, inp_image, &ntiles, &tile_pixels, &tile_counts, &arules);
    printf("Total number of unique tiles is %d\n", ntiles);

    int *cells = (int *)malloc(sizeof(int)*params->out_width*params->out_height); // output tiles
    for (i = 0; i < params->out_width*params->out_height; i++) {
         cells[i] = -1; // -1 if the cell has not collapsed yet
    }

    int *mask; // list of possible tiles for each cell (=wavefunction)
    mask = (int *)malloc(sizeof(int *)*params->out_width*params->out_height*ntiles);
    for (i = 0; i < params->out_width*params->out_height; i++) {
        for (j = 0; j < ntiles; j++) {
            mask[i*ntiles + j] = 1;
        }
    }

    int *sum_weights; // sum of the weights of all possible tiles
    sum_weights = (int *)malloc(sizeof(int)*params->out_width*params->out_height);
    int temp_sum;
    for (i = 0; i < params->out_width*params->out_height; i++) {
        temp_sum = 0;
        for (j = 0; j < ntiles; j++) {
            temp_sum += tile_counts[j];
        }
        sum_weights[i] = temp_sum;
    }

    double *sum_log_weights; // sum of the log weights of all possible tiles
    sum_log_weights = (double *)malloc(sizeof(double)*params->out_width*params->out_height);
    double temp_log_sum;
    for (i = 0; i < params->out_width*params->out_height; i++) {
        temp_log_sum = 0;
        for (j = 0; j < ntiles; j++) {
            temp_log_sum += tile_counts[j]*log(tile_counts[j]);
        }
        sum_log_weights[i] = temp_log_sum;
    }

    double *random_entropy; // random part of the entropy
    random_entropy = (double *)malloc(sizeof(double)*params->out_width*params->out_height);
    for (i = 0; i < params->out_width*params->out_height; i++) {
        random_entropy[i] = 0.01*(double)rand()/RAND_MAX;
    }

    struct adjacency *ecount; // enabler counter
    ecount = (struct adjacency *)malloc(sizeof(struct adjacency)*params->out_width*params->out_height);
    int left_count;
    int right_count;
    int top_count;
    int bottom_count;
    for (i = 0; i < ntiles; i++) {
        left_count = 0;
        right_count = 0;
        top_count = 0;
        bottom_count = 0;
        for (j = 0; j < ntiles; j++) {
            if (arules[i].left[j] == 1) {
                left_count++;
            }
            if (arules[i].right[j] == 1) {
                right_count++;
            }
            if (arules[i].top[j] == 1) {
                top_count++;
            }
            if (arules[i].bottom[j] == 1) {
                bottom_count++;
            }
        }
        for (j = 0; j < params->out_width*params->out_height; j++) {
            ecount[j].left[i] = left_count;
            ecount[j].right[i] = right_count;
            ecount[j].top[i] = top_count;
            ecount[j].bottom[i] = bottom_count;
        }
    }
//    uncomment here and below to visualize the process
//    int nstep = 0;
//    char fname[64];
    // propagating constraints
    if (params->constraints == 1) {
        struct input_params cons_params = *params;
        cons_params.inp_filename = params->constraints_filename;

        struct pixel *cons_image;
        read_from_png(&cons_params, &cons_image);
        params->out_width = cons_params.width;
        params->out_height = cons_params.height;

        int i, j;
        int ichosen, w, sum_w;
        struct pixel cons_px;

        for (i = 0; i < params->out_width*params->out_height; i++) {
            cons_px = cons_image[i];
            if (cons_px.alpha > 0) {
                for (j = 0; j < ntiles; j++) {
                    if (mask[i*ntiles + j] == 1) {
                        if ( (cons_px.R != tile_pixels[j].R) ||
                            (cons_px.G != tile_pixels[j].G) ||
                            (cons_px.B != tile_pixels[j].B) ) {
                            exclude_propagate(i, j, ntiles, tile_counts, arules, &mask, &cells, &sum_weights, &sum_log_weights, &ecount, params);
//                            nstep++;
//                            snprintf(fname, 64, "step_%d.png", nstep);
//                            dump_state(fname, tile_pixels, tile_counts, mask, *params, ntiles);
                        }
                    }
                }
            }
        }

        for (i = 0; i < params->out_width*params->out_height; i++) {
            cons_px = cons_image[i];
            if (cons_px.alpha > 0) {
                sum_w = 0;
                for (j = 0; j < ntiles; j++) {
                    if (mask[i*ntiles + j] == 1) {
                        if ( (cons_px.R == tile_pixels[j].R) &&
                            (cons_px.G == tile_pixels[j].G) &&
                            (cons_px.B == tile_pixels[j].B) ) {
                            sum_w += tile_counts[j];
                        }
                    }
                }
                if (sum_w == 0) {
                    printf("ERROR: failed to satisfy constraints\n");
                    return -1;
                }
                w = rand() % sum_w;
                sum_w = 0;
                for (j = 0; j < ntiles; j++) {
                    if (mask[i*ntiles + j] == 1) {
                        if ( (cons_px.R == tile_pixels[j].R) &&
                            (cons_px.G == tile_pixels[j].G) &&
                            (cons_px.B == tile_pixels[j].B) ) {
                            sum_w += tile_counts[j];
                            if (sum_w >= w) {
                                ichosen = j;
                                break;
                            }
                        }
                    }
                }
                j = collapse_propagate(i, ichosen, ntiles, tile_counts, arules, &mask, &cells, &sum_weights, &sum_log_weights, &ecount, params);
//                nstep++;
//                snprintf(fname, 64, "step_%d.png", nstep);
//                dump_state(fname, tile_pixels, tile_counts, mask, *params, ntiles);
                if (j == -1) {
                    printf("ERROR: failed to satisfy constraints\n");
                    return -1;
                }
                else {
//                    printf("collapsed to %d\n", ichosen);
                }
            }
        }

        free(cons_image);
    }


    // main generation loop
//    nstep++;
//    snprintf(fname, 64, "step_%d.png", nstep);
//    dump_state(fname, tile_pixels, tile_counts, mask, *params, ntiles);
    while (1 == 1) {
        i = find_next_cell(sum_weights, sum_log_weights, random_entropy, cells, params);
        if (i < 0) {
            break;
        }

        j = collapse_propagate(i, -1, ntiles, tile_counts, arules, &mask, &cells, &sum_weights, &sum_log_weights, &ecount, params);
        if (j == -1) {
            printf("ERROR: failed to converge\n");
            return -1;
        }
        else {
//            nstep++;
//            snprintf(fname, 64, "step_%d.png", nstep);
//            dump_state(fname, tile_pixels, tile_counts, mask, *params, ntiles);
        }
    }

    if ((i != -1) && (j != -1)) {
        struct pixel *out_image = (struct pixel*)malloc(sizeof(struct pixel)*params->out_width*params->out_height);
        for (k = 0; k < params->out_width*params->out_height; k++) {
            out_image[k] = tile_pixels[cells[k]];
        }
        write_to_png(*params, out_image);
        free(out_image);
    }

    // print out adjacency rules
//    for (i = 0; i < ntiles; i++) {
//        printf("----------------------\n");
//        printf("%3d  LEFT: ", i);
//        for (j = 0; j < ntiles; j++) {
//            if (arules[i].left[j] == 1) {
//                printf("%d ", j);
//            }
//        }
//        printf("\n");
//        printf("    RIGHT: ");
//        for (j = 0; j < ntiles; j++) {
//            if (arules[i].right[j] == 1) {
//                printf("%d ", j);
//            }
//        }
//        printf("\n");
//         printf("      TOP: ");
//        for (j = 0; j < ntiles; j++) {
//            if (arules[i].top[j] == 1) {
//                printf("%d ", j);
//            }
//        }
//        printf("\n");
//        printf("   BOTTOM: ");
//        for (j = 0; j < ntiles; j++) {
//            if (arules[i].bottom[j] == 1) {
//                printf("%d ", j);
//            }
//        }
//        printf("\n");
//    }

    free(inp_image);
    free(tile_pixels);
    free(arules);
    free(tile_counts);

    free(cells);
    free(mask);
    free(sum_weights);
    free(sum_log_weights);
    free(random_entropy);
    free(ecount);

    return 0;
}


int main(int argc, char** argv) {
    struct input_params params;

    params.constraints = 0;

    params.inp_filename = "samples/Cats.png";
    params.out_filename = "out.png";
    params.out_width = 60;
    params.out_height = 40;
    params.tile_size = 3;
    params.rot_aug = 0;
    params.h_aug = 1;
    params.v_aug = 0;

//    params.inp_filename = "samples/Maze.png";
//    params.out_filename = "out.png";
//    params.out_width = 50;
//    params.out_height = 30;
//    params.tile_size = 2;
//    params.rot_aug = 0;
//    params.h_aug = 0;
//    params.v_aug = 0;
//    params.constraints = 1;
//    params.constraints_filename = "samples/MazeRoute.png";

//    params.inp_filename = "samples/Flowers.png";
//    params.out_filename = "out.png";
//    params.out_width = 50;
//    params.out_height = 30;
//    params.tile_size = 2;
//    params.rot_aug = 0;
//    params.h_aug = 1;
//    params.v_aug = 0;
//    params.constraints = 1;
//    params.constraints_filename = "samples/Ground.png";

    generate_image(&params);

    return (EXIT_SUCCESS);
}

