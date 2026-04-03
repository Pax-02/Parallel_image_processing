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

### Create folders

    mkdir -p images/small/result/serial images/small/result/openmp images/small/result/mpi
    mkdir -p images/medium/result/serial images/medium/result/openmp images/medium/result/mpi
    mkdir -p images/large/result/serial images/large/result/openmp images/large/result/mpi

### Compile and Run

    gcc serial/serial_baseline.c -o serial/serial_baseline -lm
    ./serial/serial_baseline small
    ./serial/serial_baseline medium
    ./serial/serial_baseline large

### Compile and RUN the OPENMP version

    gcc -fopenmp openMP/openMP.c -o openMP/openMP -lm

#### Run on Different Threads

    OMP_NUM_THREADS=1 ./openMP/openMP small
    OMP_NUM_THREADS=2 ./openMP/openMP small
    OMP_NUM_THREADS=4 ./openMP/openMP small
    OMP_NUM_THREADS=8 ./openMP/openMP small

#### Run on Different sizes

    OMP_NUM_THREADS=4 ./openMP/openMP small
    OMP_NUM_THREADS=4 ./openMP/openMP medium
    OMP_NUM_THREADS=4 ./openMP/openMP large

### Compile and RUN the MPI version

    mpicc MPI/MPI.c -o MPI/MPI -lm
    mpirun -np [numberOfProcesses] ./MPI/MPI image_size

    Ex:
    mpirun -np 4 ./MPI/MPI small
    mpirun -np 4 ./MPI/MPI medium
    mpirun -np 4 ./MPI/MPI large

### Github Link :

    https://github.com/Pax-02/Parallel_image_processing.git
