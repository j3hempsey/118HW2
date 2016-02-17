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

	gil::rgb8_image_t img(height, width);
	auto img_view = gil::view(img);

	if (rank == 0) {
		y = minY;
		for (int i = 0; i < height/np; ++i) {
			x = minX;
			for (int j = 0; j < width; ++j) {
				img_view(j, i) = render(mandelbrot(x, y)/512.0);
				x += jt;
			}
			y += it;
		}
	}
	else {
		y = minY + (it * double(rank));
		for (int i = (height/np) * (rank); i < (height/np) * (rank + 1); ++i) {
			x = minX;
			for (int j = 0; j < width; ++j) {
				img_view(j, i) = render(mandelbrot(x, y)/512.0);
				x += jt;
			}
			y += (it * double(np));
		}
	}
	MPI_Barrier (MPI_COMM_WORLD); /* Synchronize the nodes */
	if (rank == 0) {
		gil::png_write_view("mandelbrot.png", const_view(img));
	}
	MPI_Finalize();
	return 0;
}

/* eof */

