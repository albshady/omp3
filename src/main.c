#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct Picture {
    // char* filetype;
    uint32_t height;
    uint32_t width;
    uint32_t max_brightness;
    unsigned char** pixels;
} Picture;

void destruct_picture(Picture picture) {
    for (int i = 0; i < picture.height; i++)
        free(picture.pixels[i]);
}

Picture read_picture(const char* filepath) {
    FILE* input_file = fopen(filepath, "rb");

    if (input_file == NULL) {
        printf("Input file not found!\n");
        exit(1);
    }

    Picture picture;
    fscanf(input_file, "P5\n%d %d\n%d", &picture.width, &picture.height, &picture.max_brightness);
    fgetc(input_file);

    unsigned char** pixels = (unsigned char**)malloc(picture.height * sizeof(unsigned char *));

    for (int i = 0; i < picture.height; i++) {
        pixels[i] = (unsigned char *)malloc(picture.width * sizeof(unsigned char));
        fread(pixels[i], sizeof(unsigned char), picture.width, input_file);
    }

    picture.pixels = pixels;
    fclose(input_file);

    return picture;
}

int calculate_box_radius(float sigma, uint32_t number_of_boxes) {
    return round(sqrt(12 * sigma * sigma / number_of_boxes + 1));
}

Picture blur_single_thread(Picture picture, uint32_t number_of_boxes, float sigma) {
    int radius = calculate_box_radius(sigma, number_of_boxes);
    return picture;
}

Picture blur_multi_thread(Picture picture, uint32_t number_of_boxes, float sigma) {
    return picture;
}

Picture blur(Picture picture, int num_threads, uint32_t number_of_boxes, float sigma) {
    if (num_threads == -1) {
        double start = omp_get_wtime();
        Picture blurred = blur_single_thread(picture, number_of_boxes, sigma);
        printf("Time (%i thread(s)): %g ms\n", 1, omp_get_wtime() - start);
        return blurred;
    }
    if (num_threads == 0) {
        num_threads = omp_get_max_threads();
    }

    omp_set_num_threads(num_threads);

    double start = omp_get_wtime();
    Picture blurred = blur_multi_thread(picture, number_of_boxes, sigma);
    printf("Time (%i thread(s)): %g ms\n", num_threads, omp_get_wtime() - start);
    return blurred;
}

int write_picture(const char* filepath, Picture picture) {
    FILE* output_file = fopen(filepath, "wb");
    if (output_file == NULL) {
        printf("Output file not found!\n");
        destruct_picture(picture);
        return 1;
    }

    fprintf(output_file, "P5\n%d %d\n%d", picture.width, picture.height, picture.max_brightness);
    fputc('\n', output_file);
    for (int i = 0; i < picture.height; i++) {
        fwrite(picture.pixels[i], sizeof(unsigned char), picture.width, output_file);
    }
    fclose(output_file);
    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc != 6) {
        printf("Command line arguments shall be <input_fp> <output_fp> <num_threads> <number_of_boxes> <sigma>\n");
        return 1;
    }

    const char* input_filepath = argv[1];
    const char* output_filepath = argv[2];
    int num_threads = atoi(argv[3]);

    if (num_threads < -1) {
        printf("Number of threads should be >= -1\n");
        return 1;
    }
    uint32_t number_of_boxes = (uint32_t)atoi(argv[4]);
    float sigma = atof(argv[5]);
    if (sigma <= 0) {
        printf("Sigma value must be > 0\n");
        return 1;
    }

    Picture picture = read_picture(input_filepath);
    Picture blurred = blur(picture, num_threads, number_of_boxes, sigma);

    int return_code = write_picture(output_filepath, blurred);
    destruct_picture(picture);
    // destruct_picture(blurred);

    return return_code;
}
