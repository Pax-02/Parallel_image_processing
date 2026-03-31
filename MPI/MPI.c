#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

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

void copy_image(const Image *input, Image *output) {
    output->width = input->width;
    output->height = input->height;
    output->max_value = input->max_value;
    output->data = (unsigned char *)malloc(input->width * input->height);

    for (int i = 0; i < input->width * input->height; i++) {
        output->data[i] = input->data[i];
    }
}

// Allocate memory for output
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
//swap buffers to copy data instead of loop
void swap_image_data(Image *a, Image *b) {
    unsigned char *tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}

void gaussian_blur(Image *img, Image *buf, int rank, int size){

    int *sendCounts = malloc(size*sizeof(int));
    int *displs = malloc(size*sizeof(int));

    int rows = img->height/size;
    int remainder = img->height%size;

    //setting up how many rows each processor gets
    int offset = 0;
    for (int i = 0; i<size; i++){
        if(i < remainder){
            rows++;
        }
        sendCounts[i] = rows*img->width;
        displs[i] = offset;
        offset = offset+sendCounts[i];
    }

    int imgSize = sendCounts[rank];
    int rowAmt = imgSize/img->width;

    int start = displs[rank]/img->width;

    unsigned char *dataIn = malloc(imgSize);
    unsigned char *dataOut = malloc(imgSize);

    MPI_Scatterv(img->data, sendCounts, displs, MPI_UNSIGNED_CHAR, dataIn, imgSize, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    //setting bounds for crossover area between processesors
    unsigned char *crossover = malloc((rowAmt+2)*img->width);
    for(int i = 0; i<rowAmt; i++){
        for(int j = 0; j<img->width; j++){
            crossover[(i+1)*img->width+j] = dataIn[i*img->width+j];
        }
    }

    int up,down;
    if(rank==0){up=MPI_PROC_NULL;}
    else{up=rank-1;}
    if(rank==size-1){down=MPI_PROC_NULL;}
    else{down=rank+1;}

    int send1 = 0;
    int send2 = 1;
    MPI_Sendrecv(dataIn, img->width, MPI_UNSIGNED_CHAR, up, send1, crossover+(rowAmt+1)*img->width, img->width, MPI_UNSIGNED_CHAR, down, send1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv(dataIn+(rowAmt-1)*img->width, img->width, MPI_UNSIGNED_CHAR, down, send2, crossover, img->width, MPI_UNSIGNED_CHAR, up, send2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //create guassian distribution matrix
    double gaussianDist[] = {1,2,1,2,4,2,1,2,1};    
    int index, i, average, center;

    //apply filter to input image 
    for(int y=0; y<rowAmt; y++){
        int globaly = start+y;
        for(int x=0; x<img->width; x++){
            index = (y*img->width+x);
            i = 0;
            average = 0;
            center = (y+1)*img->width+x;

            //Blurring pixels in the corners
            if(globaly==0&&x==0){
                dataOut[index] = ((crossover[center]*4)+(crossover[center+1]*2)+(crossover[center+img->width]*2)+(crossover[center+img->width+1]))/9;
            }
            else if(globaly==0&&x==img->width-1){
                dataOut[index] = ((crossover[center]*4)+(crossover[center-1]*2)+(crossover[center+img->width]*2)+(crossover[center+img->width-1]))/9;
            }
            else if(globaly==img->height-1&&x==0){
                dataOut[index] = ((crossover[center]*4)+(crossover[center+1]*2)+(crossover[center-img->width]*2)+(crossover[center-img->width+1]))/9;
            }
            else if(globaly==img->height-1&&x==img->width-1){
                dataOut[index] = ((crossover[center]*4)+(crossover[center-1]*2)+(crossover[center-img->width]*2)+(crossover[center-img->width-1]))/9;
            }
            //Blurring pixels on the edges
            else if(globaly==0){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if(i>=3){
                            average = average + (crossover[center+y2*img->width+x2]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                dataOut[index] = average/12;
            }
            else if(globaly==img->height-1){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if(i<6){
                            average = average + (crossover[center+y2*img->width+x2]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                dataOut[index] = average/12;
            }
            else if(x==0){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if((i%3)!=0){
                            average = average + (crossover[center+y2*img->width+x2]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                dataOut[index] = average/12;
            }
            else if(x==img->width-1){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if((i%3)!=2){
                            average = average + (crossover[center+y2*img->width+x2]*gaussianDist[i]);
                        }
                        i++;
                    }
                }
                dataOut[index] = average/12;
            }
            //Blurring pixels in the middle
            else{
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        average = average + (crossover[center+y2*img->width+x2]*gaussianDist[i]);
                        i++;
                    }
                }
                dataOut[index] = average/16;
            }
        }
    }

    MPI_Gatherv(dataOut, imgSize, MPI_UNSIGNED_CHAR, buf->data, sendCounts, displs, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    free(dataIn);
    free(dataOut);
    free(crossover);
    free(sendCounts);
    free(displs);
}

//clear the memory (image)
void free_image(Image *img) {
    free(img->data);
    img->data = NULL;
}

int main (int argc, char** argv){
    MPI_Init(&argc, &argv);
    int rank, size;
    double start, end;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Image input = {0};
    Image output = {0};

    if(rank==0){
        //load the image
        if (!load_pgm("Images/small/starter/source.pgm",&input)){
            printf("Couldn't load the image\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        //allocate output once with same size as input
        init_image_like(&input, &output);
    }

    MPI_Bcast(&input.width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&input.height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&input.max_value, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank!=0){
        input.data = malloc(input.width*input.height);
        output.data = malloc(input.width*input.height);
    }

    if(rank==0){
        start = MPI_Wtime();
    }
    gaussian_blur(&input, &output, rank, size);
    if(rank==0){
        end = MPI_Wtime();
        printf("Gaussian Blur Time: %fs\n", end-start);
    }

    if(rank==0){
        //store the gausian image 
        if (!save_pgm("Images/small/result/mpi/gaussian.pgm", &output)) {
            printf("Failed to save gaussian blur image.\n");
            free_image(&input);
            free_image(&output);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    
    /*
    //setting the blurred image as the input
    swap_image_data(&input, &output);

    median_filter(&input, &output);

    // Store the median image 
    if (!save_pgm("images/small/result/serial/median.pgm", &output)) {
        printf("Failed to save median filter image.\n");
        free_image(&input);
        free_image(&output);
        return 1;
    }
    //set median image as the new input
    swap_image_data(&input, &output);

    sobel(&input, &output);

    if (!save_pgm("images/small/result/serial/sobel.pgm", &output)) {
        printf("Failed to save image.\n");
        free_image(&input);
        free_image(&output);
        return 1;
    }*/
    if(rank==0){
        free_image(&input);
        free_image(&output);
    }
    if(rank!=0){
        free(input.data);
        free(output.data);
    }

    MPI_Finalize();
    return 0;
}