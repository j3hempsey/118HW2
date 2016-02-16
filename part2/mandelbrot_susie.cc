/**
 *  \file mandelbrot_susie.cc
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
  /* Lucky you, you get to write MPI code */
  double minX = -2.1;
  double maxX = 0.7;
  double minY = -1.25;
  double maxY = 1.25;
  int height, width;
  int rank, size;
  int buffer;

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
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  printf("Entering switch, size: %d\n", size);
  if (rank == 0)
  {
    //Master
    MPI_Status status;
    int count;
    
    gil::rgb8_image_t img(height, width);
    auto img_view = gil::view(img);
    
    float **data_array;
    data_array = contig2dArray(width, height);
    float **tempdata;
    tempdata = contig2dArray(width, height);

    //Recieve all of the data from all of the processes
    while (count < size - 1){
      //height / size will be the max height array we will recieve
      printf("waiting for recv\n");
      MPI_Recv(&(tempdata[0][0]), width * (height), // / size,
        MPI_FLOAT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
      printf("recieved\n");

      for (int i = 0; i < height; ++i)
      {
        for (int j = 0; j < width; ++j)
        {
          img_view(j, i) = render(tempdata[j][i]);
        }
      }
      ++count;
    }
    free(tempdata[0]);
    free(tempdata);
    free(data_array[0]);
    free(data_array);
    gil::png_write_view("mandelbrot-test.png", const_view(img));

  }
  else
  {
    double minX = -2.1;
    double maxX = 0.7;
    double minY = -1.25;
    double maxY = 1.25;
    //Workers
    //Recv the num of rows to process
    float **data;
    data = contig2dArray(width, height); //(height - rank) / size);

    // MPI_Status status;
    // MPI_Recv(&numProc, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    printf("Calculating...\n");
    y = minY;
    for(int i = 0; i < height; ++i)
    //for(int i = rank; rank + (i * size) < height; ++i)
    {
      x = minX;
      for (int j = 0; j < width; ++j)
      {
        data[j][i] = mandelbrot(x, y) / 512.0;
        printf("%lf, ", mandelbrot(x,y)/512.0);
        
        x += jt;
      }
      y += it;
      printf("\n");
    }
    printf("Sending data...%d\n", (width * ((height ))));//- rank) / size)));
    //Send data
    MPI_Send(&(data[0][0]), (width * ((height )))//- rank)) / size)
    , MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    //free
    free(data[0]);
    free(data);
  }
  MPI_Finalize();
  return 0;
}

/* eof */
