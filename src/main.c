#include <math.h>
#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define NUMBER_OF_ITERATIONS 3


typedef struct Picture {
    uint32_t height;
    uint32_t width;
    uint32_t max_brightness;
    unsigned char** pixels;
} Picture;

void destruct_pixels(unsigned char** pixels, uint32_t height) {
    for (int i = 0; i < height; i++)
        free(pixels[i]);
}

void destruct_picture(Picture picture) {
    destruct_pixels(picture.pixels, picture.height);
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

unsigned char** blur_box_horizontally(Picture picture, int radius) {
    unsigned char** pixels = (unsigned char**)malloc(picture.height * sizeof(unsigned char *));

    for (int i = 0; i < picture.height; i++) {
        pixels[i] = (unsigned char *)malloc(picture.width * sizeof(unsigned char));
    }

    for (int i = 0; i < picture.height; i++) {
        for (int j = 0; j < picture.width; j++) {
            double brightness = 0;
            int count = 0;
            for (int x = fmax(j - radius, 0); x < fmin(j + radius + 1, picture.width); x++) {
                brightness += picture.pixels[i][x];
                count++;
            }
            pixels[i][j] = round(brightness / count);
        }
    }
    return pixels;
}

unsigned char** blur_box_vertically(Picture picture, int radius) {
    unsigned char** pixels = (unsigned char**)malloc(picture.height * sizeof(unsigned char *));

    for (int i = 0; i < picture.height; i++) {
        pixels[i] = (unsigned char *)malloc(picture.width * sizeof(unsigned char));
    }

    for (int i = 0; i < picture.height; i++) {
        for (int j = 0; j < picture.width; j++) {
            double brightness = 0;
            int count = 0;
            for (int y = fmax(i - radius, 0); y < fmin(i + radius + 1, picture.height); y++) {
                brightness += picture.pixels[y][j];
                count++;
            }
            pixels[i][j] = round(brightness / count);
        }
    }
    return pixels;
}

Picture copy_picture(Picture original) {
    Picture copied;
    copied.height = original.height;
    copied.width = original.width;
    copied.max_brightness = original.max_brightness;

    unsigned char** pixels = (unsigned char**)malloc(original.height * sizeof(unsigned char *));

    for (int i = 0; i < original.height; i++) {
        pixels[i] = (unsigned char *)malloc(original.width * sizeof(unsigned char));
        for (int j = 0; j < original.width; j++) {
            pixels[i][j] = original.pixels[i][j];
        }
    }
    copied.pixels = pixels;

    return copied;
}

Picture blur_single_thread(Picture picture, uint32_t number_of_boxes, float sigma) {
    int radius = calculate_box_radius(sigma, number_of_boxes);
    Picture blurred = copy_picture(picture);
    unsigned char** pixels;

    for (int iteration = 0; iteration < NUMBER_OF_ITERATIONS; iteration++) {
        pixels = blur_box_horizontally(blurred, radius);
        destruct_pixels(blurred.pixels, blurred.height);
        blurred.pixels = pixels;

        pixels = blur_box_vertically(blurred, radius);
        destruct_pixels(blurred.pixels, blurred.height);
        blurred.pixels = pixels;
    }

    return blurred;
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
    destruct_picture(blurred);

    return return_code;
}
