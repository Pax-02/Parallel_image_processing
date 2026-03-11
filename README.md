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

    gcc src/serial_baseline.c -o serial_baseline

    ./serial_baseline
