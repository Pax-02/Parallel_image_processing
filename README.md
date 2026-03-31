# Image Parallel Programming Project

## First Step

### Converting JPG to PGM

For this project, images must be in PGM (Portable GrayMap) Specifically P2 format so they can be easily processed . A convenient way to convert images is by using ImageMagick.

#### Install ImageMagick

##### Ubuntu / Debian

    sudo apt install imagemagick

##### macOS

    brew install imagemagick

##### Convert JPG to PGM command

    magick image.jpg -compress none image.pgm

## Second Step

Compile and Run

    gcc serial/serial_baseline.c -o serial/serial_baseline -lm

    ./serial/serial_baseline

### Github Link :

    https://github.com/Pax-02/Parallel_image_processing.git

### Compile and RUN the OPENMP version

    gcc -fopenmp openmp/openMP.c -o openMP/openMP -lm

#### Run on Different Threads

    OMP_NUM_THREADS=1 ./openMP/openMP
    OMP_NUM_THREADS=2 ./openMP/openMP
    OMP_NUM_THREADS=4 ./openMP/openMP
    OMP_NUM_THREADS=8 ./openMP/openMP

### Compile and RUN the MPI version
    mpicc MPI/MPI.c -o MPI/MPI -lm
    mpirun -np [numberOfProcesses] ./MPI/MPI