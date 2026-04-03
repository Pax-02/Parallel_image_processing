#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <omp.h>

//define the structure
typedef struct {
    int width;
    int height;
    int max_value;
    unsigned char *data;
} Image;

//load the image (pgm)
int load_pgm(const char *filename, Image *img){
    FILE *file = fopen(filename,"r");
    if (!file){
        perror("Error opening the file");
        return 0;
    }

    char format[3];
    if (fscanf(file, "%2s", format) != 1) {
        fclose(file);
        return 0;
    }

    if (strcmp(format, "P2") != 0) {
        printf("Only P2 PGM format is supported.\n");
        fclose(file);
        return 0;
    }

    if (fscanf(file, "%d %d", &img->width, &img->height) != 2) {
        fclose(file);
        return 0;
    }

    if (fscanf(file, "%d", &img->max_value) != 1) {
        fclose(file);
        return 0;
    }

    img->data = (unsigned char *)malloc(img->width * img->height);
    if (!img->data) {
        fclose(file);
        return 0;
    }

    for (int i = 0; i < img->width * img->height; i++) {
        int pixel;
        if (fscanf(file, "%d", &pixel) != 1) {
            free(img->data);
            fclose(file);
            return 0;
        }
        img->data[i] = (unsigned char)pixel;
    }

    fclose(file);
    return 1;
}

//save a PGM
int save_pgm(const char *filename, const Image *img){
    printf("Saving to: [%s]\n", filename);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening output file");
        return 0;
    }

    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", img->width, img->height);
    fprintf(file, "%d\n", img->max_value);

    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            fprintf(file, "%d ", img->data[i * img->width + j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return 1;
}

//Copy image function to use as buffer
void copy_image(const Image *input, Image *output) {
    
    int i;
    #pragma omp single
    {
        output->width = input->width;
        output->height = input->height;
        output->max_value = input->max_value;
        output->data = (unsigned char *)malloc(input->width * input->height);
    }

    //parallel copy because each element is independent
   #pragma omp for schedule(static) private(i)
    for (i = 0; i < input->width * input->height; i++) {
        output->data[i] = input->data[i];
    }
}
//Initialize and allocate memory for the image (output) <<to help not to use copy_image that had to always allocate memory
void init_image_like(const Image *src, Image *dst) {
    dst->width = src->width;
    dst->height = src->height;
    dst->max_value = src->max_value;
    dst->data = (unsigned char *)malloc(src->width * src->height);

    if (!dst->data) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
}

//Swap the data pointers instead of looping to copy
void swap_image_data(Image *a, Image *b) {
    unsigned char *tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}


// Apply the Gaussian Blur Filter
void gaussian_blur(Image *img, Image *buf){

    double gaussianDist[] = {1,2,1,2,4,2,1,2,1};
    int y, x;

    //parallelize the outer image loops
    #pragma omp for collapse(2) schedule(static) private(y, x)
    for(y=0; y<img->height; y++){
        for(x=0; x<img->width; x++){

            int i;
            int index;
            double average = 0;

            index = (y*img->width+x);
            i = 0;
            average = 0;

            //Blurring pixels in the corners
            if(y==0&&x==0){
                buf->data[index] = ((img->data[index]*4)+(img->data[index+1]*2)+(img->data[index+img->width]*2)+(img->data[index+img->width+1]))/9;
            }
            else if(y==0&&x==img->width-1){
                buf->data[index] = ((img->data[index]*4)+(img->data[index-1]*2)+(img->data[index+img->width]*2)+(img->data[index+img->width-1]))/9;
            }
            else if(y==img->height-1&&x==0){
                buf->data[index] = ((img->data[index]*4)+(img->data[index+1]*2)+(img->data[index-img->width]*2)+(img->data[index-img->width+1]))/9;
            }
            else if(y==img->height-1&&x==img->width-1){
                buf->data[index] = ((img->data[index]*4)+(img->data[index-1]*2)+(img->data[index-img->width]*2)+(img->data[index-img->width-1]))/9;
            }
            //Blurring pixels on the edges
            else if(y==0){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if(i>=3){
                            average = average + (img->data[(index+y2*img->width+x2)]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                buf->data[index] = average/12;
            }
            else if(y==img->height-1){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if(i<6){
                            average = average + (img->data[(index+y2*img->width+x2)]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                buf->data[index] = average/12;
            }
            else if(x==0){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if((i%3)!=0){
                            average = average + (img->data[(index+y2*img->width+x2)]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                buf->data[index] = average/12;
            }
            else if(x==img->width-1){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if((i%3)!=2){
                            average = average + (img->data[(index+y2*img->width+x2)]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                buf->data[index] = average/12;
            }
            //Blurring pixels in the middle
            else{
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        average = average + (img->data[(index+y2*img->width+x2)]*gaussianDist[i]);
                        i++;
                    }
                }
                buf->data[index] = average/16;
            }
        }
    }
}

//sorting function to be used in median
void sort_pixels(unsigned char arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[i]) {
                unsigned char temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
}

void median_filter(Image *img, Image *buf) {
    //each pixel can be processed independently
    int y, x;
    #pragma omp for collapse(2) schedule(static) private(y, x)
    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {

            unsigned char window[9];
            int count = 0;

            for (int y2 = -1; y2 <= 1; y2++) {
                for (int x2 = -1; x2 <= 1; x2++) {
                    int ny = y + y2;
                    int nx = x + x2;

                    if (ny >= 0 && ny < img->height && nx >= 0 && nx < img->width) {
                        window[count] = img->data[ny * img->width + nx];
                        count++;
                    }
                }
            }

            sort_pixels(window, count);
            buf->data[y * img->width + x] = window[count / 2];
        }
    }
}

void sobel(Image *img, Image *buf){

    double matrixx[] = {-1,0,1,-2,0,2,-1,0,1};
    double matrixy[] = {1,2,1,0,0,0,-1,-2,-1};

    int y, x;
    //parallelize the per-pixel work
    #pragma omp for collapse(2) schedule(static) private(y, x)
    for(y=0; y<img->height; y++){
        for(x=0; x<img->width; x++){

            int i;
            int index;
            double averagex = 0;
            double averagey = 0;

            index = (y*img->width+x);
            i = 0;
            averagex = 0;
            averagey = 0;

            //Applying to pixels in the corners
            if(y==0&&x==0){
                averagex = ((img->data[index]*0)+(img->data[index+1]*2)+(img->data[index+img->width]*0)+(img->data[index+img->width+1*1]));
                averagey = ((img->data[index]*0)+(img->data[index+1]*0)+(img->data[index+img->width]*-2)+(img->data[index+img->width+1*-1]));
            }
            else if(y==0&&x==img->width-1){
                averagex = ((img->data[index]*0)+(img->data[index-1]*-2)+(img->data[index+img->width]*0)+(img->data[index+img->width-1*-1]));
                averagey = ((img->data[index]*0)+(img->data[index-1]*0)+(img->data[index+img->width]*-2)+(img->data[index+img->width-1*-1]));
            }
            else if(y==img->height-1&&x==0){
                averagex = ((img->data[index]*0)+(img->data[index+1]*2)+(img->data[index-img->width]*0)+(img->data[index-img->width+1*1]));
                averagey = ((img->data[index]*0)+(img->data[index+1]*0)+(img->data[index-img->width]*2)+(img->data[index-img->width+1*1]));
            }
            else if(y==img->height-1&&x==img->width-1){
                averagex = ((img->data[index]*0)+(img->data[index-1]*-2)+(img->data[index-img->width]*0)+(img->data[index-img->width-1*-1]));
                averagey = ((img->data[index]*0)+(img->data[index-1]*0)+(img->data[index-img->width]*2)+(img->data[index-img->width-1*1]));
            }
            //Applying to pixels on the edges
            else if(y==0){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if(i>=3){
                            averagex = averagex + (img->data[(index+y2*img->width+x2)]*matrixx[i]);
                            averagey = averagey + (img->data[(index+y2*img->width+x2)]*matrixy[i]);
                        }
                        i++;
                    }
                }
            }
            else if(y==img->height-1){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if(i<6){
                            averagex = averagex + (img->data[(index+y2*img->width+x2)]*matrixx[i]);
                            averagey = averagey + (img->data[(index+y2*img->width+x2)]*matrixy[i]);
                        }
                        i++;
                    }
                }
            }
            else if(x==0){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if((i%3)!=0){
                            averagex = averagex + (img->data[(index+y2*img->width+x2)]*matrixx[i]);
                            averagey = averagey + (img->data[(index+y2*img->width+x2)]*matrixy[i]);
                        }
                        i++;
                    }
                }
            }
            else if(x==img->width-1){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if((i%3)!=2){
                            averagex = averagex + (img->data[(index+y2*img->width+x2)]*matrixx[i]);
                            averagey = averagey + (img->data[(index+y2*img->width+x2)]*matrixy[i]);
                        }
                        i++;
                    }
                }
            }
            //Applying to pixels in the middle
            else{
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        averagex = averagex + (img->data[(index+y2*img->width+x2)]*matrixx[i]);
                        averagey = averagey + (img->data[(index+y2*img->width+x2)]*matrixy[i]);
                        i++;
                    }
                }
            }

            buf->data[index] = sqrt((averagex*averagex)+(averagey*averagey));
        }
    }
}

//clear the memory (image)
void free_image(Image *img) {
    free(img->data);
    img->data = NULL;
}

int main(int argc, char *argv[]){
    Image input = {0};
    Image output = {0};
    struct timespec start, end;

    const char *image_size = "small";
	
	if (argc > 1) {
        if (strcmp(argv[1], "small") == 0 ||
            strcmp(argv[1], "medium") == 0 ||
            strcmp(argv[1], "large") == 0) {
            image_size = argv[1];
        } else {
            printf("Usage: %s [small|medium|large]\n", argv[0]);
            return 1;
        }
    }

    char input_path[256];
    char gaussian_path[256];
    char median_path[256];
    char sobel_path[256];

    snprintf(input_path, sizeof(input_path),
             "images/%s/starter/source.pgm", image_size);

    snprintf(gaussian_path, sizeof(gaussian_path),
             "images/%s/result/openmp/gaussian.pgm", image_size);

    snprintf(median_path, sizeof(median_path),
             "images/%s/result/openmp/median.pgm", image_size);

    snprintf(sobel_path, sizeof(sobel_path),
             "images/%s/result/openmp/sobel.pgm", image_size);

	//load the image before starting the parallel region
    if (!load_pgm(input_path,&input)){
        printf("Couldn't load the image\n");
        return 1;
    }

    //allocate output once, same size as input
    init_image_like(&input, &output);

    #pragma omp parallel default(none) shared(input, output, start, end, image_size, input_path, gaussian_path, median_path, sobel_path)
    {

        //Gaussian Blur
        #pragma omp single
        clock_gettime(CLOCK_MONOTONIC, &start);

        gaussian_blur(&input, &output);

        #pragma omp single
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            printf("Gaussian Blur Time: %.10lfs\n",
                   (end.tv_sec-start.tv_sec) + (end.tv_nsec-start.tv_nsec)/1000000000.0);

            if (!save_pgm(gaussian_path, &output)) {
                printf("Failed to save gaussian blur image.\n");
            }
             //next stage read the gaussian result as input
            swap_image_data(&input, &output);
        }

        //Median Filter
        #pragma omp single
        clock_gettime(CLOCK_MONOTONIC, &start);

        median_filter(&input, &output);

        #pragma omp single
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            printf("Median Filter Time: %.10lfs\n",
                   (end.tv_sec-start.tv_sec) + (end.tv_nsec-start.tv_nsec)/1000000000.0);

            if (!save_pgm(median_path, &output)) {
                printf("Failed to save median filter image.\n");
            }
            //next stage read the median result as input
            swap_image_data(&input, &output);
        }

        //Sobel
        #pragma omp single
        clock_gettime(CLOCK_MONOTONIC, &start);

        sobel(&input, &output);

        #pragma omp single
        {
            clock_gettime(CLOCK_MONOTONIC, &end);
            printf("Sobel Edge Detection Time: %.10lfs\n",
                   (end.tv_sec-start.tv_sec) + (end.tv_nsec-start.tv_nsec)/1000000000.0);

            if (!save_pgm(sobel_path, &output)) {
                printf("Failed to save image.\n");
            }
        }
    }

    free_image(&input);
    free_image(&output);
    return 0;
}