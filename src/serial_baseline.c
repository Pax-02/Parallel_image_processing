#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//define the structure
typedef struct {
    int width;
    int height;
    int max_value;
    unsigned char *data;
} Image;

//load the image (pgm)

int load_pgm(const char *filename, Image *img){
    //open the file in read mode
    FILE *file = fopen(filename,"r");
    //check if file exist
    if (!file){
        perror("Error opening the file");
        return 0;
    }

    //check if it's a p2 PGM 
    char format[3];
        if (fscanf(file, "%2s", format) != 1) {
        fclose(file);
        return 0;
    }

    //chekc if it starts with P2
    if (strcmp(format, "P2") != 0) {
        printf("Only P2 PGM format is supported.\n");
        fclose(file);
        return 0;
    }

    // read & store the width & height
    if (fscanf(file, "%d %d", &img->width, &img->height) != 2) {
        fclose(file);
        return 0;
    }
    //read & store the max_value
        if (fscanf(file, "%d", &img->max_value) != 1) {
        fclose(file);
        return 0;
    }

    //alocate memory
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
    //open file in write mode
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening output file");
        return 0;
    }
    //start writng the format of a P2 pgm
    fprintf(file, "P2\n");
    fprintf(file, "%d %d\n", img->width, img->height);
    fprintf(file, "%d\n", img->max_value);

    //write the pixels
    for (int i = 0; i < img->height; i++) {
        for (int j = 0; j < img->width; j++) {
            fprintf(file, "%d ", img->data[i * img->width + j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return 1;
}

//Copy image fuction to test app // to be deleted

void copy_image(const Image *input, Image *output) {
    output->width = input->width;
    output->height = input->height;
    output->max_value = input->max_value;
    output->data = (unsigned char *)malloc(input->width * input->height);

    for (int i = 0; i < input->width * input->height; i++) {
        output->data[i] = input->data[i];
    }
}

// Apply the Guassian Blur Filter

void guassian_blur(Image *img, Image *buf){

    int i;
    int index;
    double average = 0;

    //create guassian distribution matrix
    double *guassianDist = (double*)malloc(sizeof(double)*9);
    guassianDist[0]=1,guassianDist[2]=1,guassianDist[6]=1,guassianDist[8]=1;
    guassianDist[1]=2,guassianDist[3]=2,guassianDist[5]=2,guassianDist[7]=2;
    guassianDist[4]=4;

    //apply filter to input image
    for(int y=0; y<img->height; y++){
        for(int x=0; x<img->width; x++){
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
            else if(y==img->width-1&&x==0){
                buf->data[index] = ((img->data[index]*4)+(img->data[index+1]*2)+(img->data[index-img->width]*2)+(img->data[index-img->width+1]))/9;
            }
            else if(y==img->width-1&&x==img->width-1){
                buf->data[index] = ((img->data[index]*4)+(img->data[index-1]*2)+(img->data[index-img->width]*2)+(img->data[index-img->width-1]))/9;
            }
            //Blurring pixels on the edges
            else if(y==0){
                for(int y2=-1; y2<=1; y2++){
                    for(int x2=-1; x2<=1; x2++){
                        if(i>=3){
                            average = average + (img->data[(index+y*img->width+x)]*guassianDist[i]);

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
                            average = average + (img->data[(index+y*img->width+x)]*guassianDist[i]);
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
                            average = average + (img->data[(index+y*img->width+x)]*guassianDist[i]);
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
                            average = average + (img->data[(index+y*img->width+x)]*guassianDist[i]);
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
                        average = average + (img->data[(index+y*img->width+x)]*guassianDist[i]);
                        i++;
                    }
                }
                buf->data[index] = average/16;
            }
        }
    }
    free(guassianDist);
}

//clear the memory (image)
void free_image(Image *img) {
    free(img->data);
    img->data = NULL;
}

int main (){
    Image input = {0};
    Image output = {0};

    //load the image
    if (!load_pgm("input/test1.pgm",&input)){
        printf("Couldn't load the image\n");
        return 1;
    }

    copy_image(&input, &output);

    guassian_blur(&input, &output);

    if (!save_pgm("output/result1.pgm", &output)) {
        printf("Failed to save image.\n");
        free_image(&input);
        free_image(&output);
        return 1;
    }

    free_image(&input);
    free_image(&output);
}

