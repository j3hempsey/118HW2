/**
 *  \file mandelbrot_joe.cc
 *
 *  \brief Implement your parallel mandelbrot set in this file.
 */

#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include "render.hh"

using namespace std;

#define WIDTH = 1000
#define HEIGHT = 1000

float
**contig2dArray(int rows, int cols) {
  float *data = (float *)malloc(rows*cols*sizeof(float));
    float **array= (float **)malloc(rows*sizeof(float*));
    for (int i=0; i<rows; i++)
        array[i] = &(data[cols*i]);
    return array;
}

int
mandelbrot(double x, double y) {
  int maxit = 511;
  double cx = x;
  double cy = y;
  double newx, newy;

  int it = 0;
  for (it = 0; it < maxit && (x*x + y*y) < 4; ++it) {
    newx = x*x - y*y + cx;
    newy = 2*x*y + cy;
    x = newx;
    y = newy;
  }
  return it;
}

int
main (int argc, char* argv[])
{
	double minX = -2.1;
	double maxX = 0.7;
	double minY = -1.25;
	double maxY = 1.25;

	int rank, np, height, width;
	//Array for proc and main
	float **data;
	float **procdata;

	if (argc == 3) {
		height = atoi (argv[1]);
		width = atoi (argv[2]);
		assert (height > 0 && width > 0);
	} else {
		fprintf (stderr, "usage: %s <height> <width>\n", argv[0]);
		fprintf (stderr, "where <height> and <width> are the dimensions of the image.\n");
		return -1;
	}

	double it = (maxY - minY)/height;
	double jt = (maxX - minX)/width;
	double x, y;

	//Setup mpi
	MPI_Init (&argc, &argv);	/* starts MPI */
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* Get process id */
	MPI_Comm_size (MPI_COMM_WORLD, &np);	/* Get number of processes */
	
	data = contig2dArray(width, height);
	procdata = contig2dArray(width, height);

	gil::rgb8_image_t img(height, width);
	auto img_view = gil::view(img);
	
	//MPI_Scatter(&(data[0][0]), height/np, MPI_FLOAT, &(procdata[0][0]), height/np, MPI_FLOAT, 0, MPI_COMM_WORLD);

	if (rank == 0) {
		y = minY;
		for (int i = 0; i < height/np; ++i) {
			x = minX;
			for (int j = 0; j < width; ++j) {
				procdata[j][i] = mandelbrot(x, y);
				x += jt;
			}
			y += it;
		}
	}
	else {
		y = minY + (it * (height/np) * (rank)); 
		for (int i = (height/np) * (rank); i < (height/np) * (rank + 1); ++i) { // splits by number of processes (i.e 500-999 if 1000 px and 2 processes)
			x = minX;
			for (int j = 0; j < width; ++j) {
				procdata[j][i] = mandelbrot(x, y);
				printf("%d ", y);
				x += jt;
			}
			y += it;
		}
	}
	
	MPI_Gather(&(procdata[0][0]), height/np, MPI_FLOAT, &(data[0][0]), height/np, MPI_FLOAT, 0, MPI_COMM_WORLD);
	
	if (rank == 0) {
		for (int i = 0; i < height; ++i) {
			for (int j = 0; j < width; ++j) {
				 img_view(j, i) = render(data[j][i]/512.0);
			}
		}
	}
	
	free(procdata[0]);
    free(procdata);
    free(data[0]);
    free(data);

	gil::png_write_view("mandelbrot.png", const_view(img));
	
	MPI_Finalize();
	return 0;
}

/* eof */

