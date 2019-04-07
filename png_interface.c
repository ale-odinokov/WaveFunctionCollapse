#include <stdlib.h>
#include <png.h>

#include "png_interface.h"
#include "datastruct.h"

int read_from_png(struct input_params *params, struct pixel **image) {
    png_byte color_type;
    png_byte bit_depth;
    int i;
    png_bytep *row_pointers;

    char header[8];
    FILE* fp = fopen(params->inp_filename, "rb");
    if(!fp) {
        puts("ERROR: Cannot open input file");
        abort();
    }
    fread(header, 1, 8, fp);
    int is_png = !png_sig_cmp(header, 0, 8);
    if(!is_png) {
        puts("ERROR: Input file is not in PNG format");
        abort();
    }
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) abort();
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        abort();
    }
    if (setjmp(png_jmpbuf(png_ptr))) abort();

    png_set_sig_bytes(png_ptr, 8);
    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    params->width = png_get_image_width(png_ptr, info_ptr);
    params->height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if(bit_depth == 16)
        png_set_strip_16(png_ptr);

    if(color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png_ptr);
    }

    png_read_update_info(png_ptr, info_ptr);

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * params->height);
    for(int y = 0; y < params->height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));
    }

    png_read_image(png_ptr, row_pointers);

    fclose(fp);

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    png_ptr = NULL;
    info_ptr = NULL;


    *image = (struct pixel *)malloc(sizeof(struct pixel) * params->height * params->width);
    for(int y = 0; y < params->height; y++) {
        png_bytep row = row_pointers[y];
        for(int x = 0; x < params->width; x++) {
            png_bytep px = &(row[x * 4]);
            i = (params->width*y + x);
            (*image)[i].R = px[0];
            (*image)[i].G = px[1];
            (*image)[i].B = px[2];
            (*image)[i].alpha = px[3];
        }
    }

    for(int y = 0; y < params->height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    return 0;
}

int write_to_png(struct input_params params, struct pixel *image) {
    int x, y;

    FILE *fp = fopen(params.out_filename, "wb");
    if (!fp) abort();

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) abort();

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) abort();

    if (setjmp(png_jmpbuf(png_ptr))) abort();

    png_init_io(png_ptr, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png_ptr,
        info_ptr,
        params.out_width, params.out_height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png_ptr, info_ptr);

//    png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

    png_bytep row = (png_bytep) malloc(3 * params.out_width * sizeof(png_byte));
    for (y = 0; y < params.out_height; y++) {
        for (x = 0; x < params.out_width; x++) {
            row[3*x] = image[params.out_width*y + x].R;
            row[3*x+1] = image[params.out_width*y + x].G;
            row[3*x+2] = image[params.out_width*y + x].B;
        }
      png_write_row(png_ptr, row);
    }
    png_write_end(png_ptr, NULL);

    if (fp != NULL) fclose(fp);
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    if (row != NULL) free(row);

    return 0;
}

int extract_tiles(struct input_params *params, struct pixel *image, int *ntiles,
                    struct pixel **tile_pixels, int **tile_counts, struct adjacency **arules) {
    int x, y, i, j, k, ii, jj, n;
    char fname[64];

    int nsquare = params->tile_size*params->tile_size;  // number of pixels in the tile
    int nutiles = params->width*params->height; // total number of non-unique tiles
    struct pixel *tiles = (struct pixel *)malloc(sizeof(struct pixel)*nutiles*nsquare);
    // adjacency rules for non-unique tiles
    struct adjacency *adj = (struct adjacency *)malloc(sizeof(struct adjacency)*nutiles);

    // store all tiles in an array
    for (y = 0; y < params->height; y++) {
        for (x = 0; x < params->width; x++) {
            for (j = 0; j < params->tile_size; j++) {
                for (i = 0; i < params->tile_size; i++) {
                    jj = (y + j) % params->height;
                    ii = (x + i) % params->width;
                    tiles[(y*params->width + x)*nsquare + j*params->tile_size + i] = image[jj*params->width + ii];
                }
            }
        }
    }

    // constructing non-unique adjacency rules by looping over 4 neighbors
    for (i = 0; i < nutiles; i++) {
        for (j = 0; j < MAX_TILES; j++) {
            adj[i].left[j] = 0;
            adj[i].right[j] = 0;
            adj[i].top[j] = 0;
            adj[i].bottom[j] = 0;
        }
    }
    int xx, yy;
    for (i = 0; i < nutiles; i++) {
        x = i % params->width;
        y = i / params->width;
        xx = (params->width + x - 1) % params->width;
        ii = y*params->width + xx;
        adj[i].left[ii] = 1;
        adj[ii].right[i] = 1;
        xx = (x + 1) % params->width;
        ii = y*params->width + xx;
        adj[i].right[ii] = 1;
        adj[ii].left[i] = 1;
        yy = (params->height + y - 1) % params->height;
        ii = yy*params->width + x;
        adj[i].top[ii] = 1;
        adj[ii].bottom[i] = 1;
        yy = (y + 1) % params->height;
        ii = yy*params->width + x;
        adj[i].bottom[ii] = 1;
        adj[ii].top[i] = 1;
    }

    // dataset augmentation, if requested
    if (params->rot_aug == 1) {
        if (nutiles*4 > MAX_TILES) {
            printf("ERROR: Number of tiles is greater than maximum value\n");
            return -1;
        }
        tiles = (struct pixel *)realloc(tiles, sizeof(struct pixel)*nutiles*nsquare*4);
        for (i = 0; i < nutiles; i++) {
            for (j = 0; j < nsquare; j++) {
                x = j % params->tile_size;
                y = j / params->tile_size;
                jj = x*params->tile_size + params->tile_size - y - 1;
                tiles[(nutiles+i)*nsquare + jj] = tiles[i*nsquare + j];
                jj = (params->tile_size - y - 1)*params->tile_size + params->tile_size - x - 1;
                tiles[(2*nutiles+i)*nsquare + jj] = tiles[i*nsquare + j];
                jj = (params->tile_size - x - 1)*params->tile_size + y;
                tiles[(3*nutiles+i)*nsquare + jj] = tiles[i*nsquare + j];
            }
        }
        adj = (struct adjacency *)realloc(adj, sizeof(struct adjacency)*nutiles*4);
        for (i = 0; i < nutiles; i++) {
            for (j = 0; j < nutiles; j++) {
                adj[nutiles+i].left[nutiles+j] = adj[i].bottom[j];
                adj[nutiles+i].right[nutiles+j] = adj[i].top[j];
                adj[nutiles+i].top[nutiles+j] = adj[i].left[j];
                adj[nutiles+i].bottom[nutiles+j] = adj[i].right[j];
                adj[2*nutiles+i].left[2*nutiles+j] = adj[i].right[j];
                adj[2*nutiles+i].right[2*nutiles+j] = adj[i].left[j];
                adj[2*nutiles+i].top[2*nutiles+j] = adj[i].bottom[j];
                adj[2*nutiles+i].bottom[2*nutiles+j] = adj[i].top[j];
                adj[3*nutiles+i].left[3*nutiles+j] = adj[i].top[j];
                adj[3*nutiles+i].right[3*nutiles+j] = adj[i].bottom[j];
                adj[3*nutiles+i].top[3*nutiles+j] = adj[i].right[j];
                adj[3*nutiles+i].bottom[3*nutiles+j] = adj[i].left[j];
            }
        }
        nutiles = 4*nutiles;
    }
    if (params->h_aug == 1) {
        if (nutiles*2 > MAX_TILES) {
            printf("ERROR: Number of tiles is greater than maximum value\n");
            return -1;
        }
        tiles = (struct pixel *)realloc(tiles, sizeof(struct pixel)*nutiles*nsquare*2);
        for (i = 0; i < nutiles; i++) {
            for (j = 0; j < nsquare; j++) {
                x = j % params->tile_size;
                y = j / params->tile_size;
                jj = y*params->tile_size + params->tile_size - x - 1;
                tiles[(nutiles+i)*nsquare + jj] = tiles[i*nsquare + j];
            }
        }
        adj = (struct adjacency *)realloc(adj, sizeof(struct adjacency)*nutiles*2);
        for (i = 0; i < nutiles; i++) {
            for (j = 0; j < nutiles; j++) {
                adj[nutiles+i].left[nutiles+j] = adj[i].right[j];
                adj[nutiles+i].right[nutiles+j] = adj[i].left[j];
                adj[nutiles+i].top[nutiles+j] = adj[i].top[j];
                adj[nutiles+i].bottom[nutiles+j] = adj[i].bottom[j];
            }
        }
        nutiles = 2*nutiles;
    }
    if (params->v_aug == 1) {
        if (nutiles*2 > MAX_TILES) {
            printf("ERROR: Number of tiles is greater than maximum value\n");
            return -1;
        }
        tiles = (struct pixel *)realloc(tiles, sizeof(struct pixel)*nutiles*nsquare*2);
        for (i = 0; i < nutiles; i++) {
            for (j = 0; j < nsquare; j++) {
                x = j % params->tile_size;
                y = j / params->tile_size;
                jj = (params->tile_size - y - 1)*params->tile_size + x;
                tiles[(nutiles+i)*nsquare + jj] = tiles[i*nsquare + j];
            }
        }
        adj = (struct adjacency *)realloc(adj, sizeof(struct adjacency)*nutiles*2);
        for (i = 0; i < nutiles; i++) {
            for (j = 0; j < nutiles; j++) {
                adj[nutiles+i].left[nutiles+j] = adj[i].left[j];
                adj[nutiles+i].right[nutiles+j] = adj[i].right[j];
                adj[nutiles+i].top[nutiles+j] = adj[i].bottom[j];
                adj[nutiles+i].bottom[nutiles+j] = adj[i].top[j];
            }
        }
        nutiles = 2*nutiles;
    }

    int same;
    struct pixel px1, px2;
    // unique indices
    int *ui = (int *)malloc(sizeof(int)*nutiles*nsquare);
    *ntiles = 0;
    // checking for repeating tiles
    for (i = 0; i < nutiles; i++) {
        same = 1;
        for (j = 0; j < i; j++) {
            same = 1;
            for (k = 0; k < nsquare; k++) {
                px1 = tiles[j*nsquare + k];
                px2 = tiles[i*nsquare + k];
                if ((px1.R != px2.R) || (px1.G != px2.G) || (px1.B != px2.B)) {
                    same = 0;
                    break;
                }
            }
            if (same == 1) {
                break;
            }
        }
        if ((same == 1) && (i != 0)) {
            ui[i] = ui[j];
        }
        else {
            ui[i] = *ntiles;
//            uncomment to print out tiles
//            snprintf(fname, 64, "tile_%d.png", *ntiles);
//            struct input_params local_params = *params;
//            local_params.out_filename = fname;
//            local_params.out_width = params->tile_size;
//            local_params.out_height = params->tile_size;
//            write_to_png(local_params, tiles+i*nsquare);
            (*ntiles)++;
        }
    }

    (*tile_pixels) = (struct pixel *)malloc(sizeof(struct pixel)*(*ntiles));
    (*arules) = (struct adjacency *)malloc(sizeof(struct adjacency)*MAX_TILES*nsquare);
    (*tile_counts) = (int *)malloc(sizeof(int)*(*ntiles)*nsquare);
    for (i = 0; i < (*ntiles); i++) {
        (*tile_counts)[i] = 0;
        for (j = 0; j < MAX_TILES; j++) {
            (*arules)[i].left[j] = 0;
            (*arules)[i].right[j] = 0;
            (*arules)[i].top[j] = 0;
            (*arules)[i].bottom[j] = 0;
        }
    }

    // collecting statistics for unique tiles
    for (i = 0; i < nutiles; i++) {
        ii = ui[i];
        (*tile_counts)[ii]++;
        (*tile_pixels)[ii] = tiles[i*nsquare];
        for (j = 0; j < MAX_TILES; j++) {
            if (adj[i].left[j] == 1)
                (*arules)[ii].left[ui[j]] = 1;
            if (adj[i].right[j] == 1)
                (*arules)[ii].right[ui[j]] = 1;
            if (adj[i].top[j] == 1)
                (*arules)[ii].top[ui[j]] = 1;
            if (adj[i].bottom[j] == 1)
                (*arules)[ii].bottom[ui[j]] = 1;
        }
    }

    free(tiles);
    free(adj);
    free(ui);
    return 0;
}

int dump_state(char *fname, struct pixel *tile_pixels, int *tile_counts, int *mask,
                struct input_params params, int ntiles) {

    struct pixel *out_image = (struct pixel*)malloc(sizeof(struct pixel)*params.out_width*params.out_height);
    for (int i = 0; i < params.out_width*params.out_height; i++) {
        int sumR = 0;
        int sumG = 0;
        int sumB = 0;
        int count = 0;
        for (int j = 0; j < ntiles; j++) {
            if (mask[i*ntiles + j] == 1) {
                count += tile_counts[j];
                sumR += tile_pixels[j].R*tile_counts[j];
                sumG += tile_pixels[j].G*tile_counts[j];
                sumB += tile_pixels[j].B*tile_counts[j];
            }
        }
        struct pixel pix;
        pix.R = sumR/count;
        pix.G = sumG/count;
        pix.B = sumB/count;
        pix.alpha = 0;
        out_image[i] = pix;
    }

    struct input_params dump_params = params;
    dump_params.out_filename = fname;
    write_to_png(dump_params, out_image);
    free(out_image);

    return 0;
}