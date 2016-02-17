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
  int *scatter_array, *disp;
  MPI_Datatype jump;

  //Array for proc and main
  float **data;
  float **procdata;

  //Alloc scatter/gather arrays
  scatter_array = (int *)malloc(sizeof(int)*size);
  disp = (int *)malloc(sizeof(int)*size);

  //Image construct
  gil::rgb8_image_t img(height, width);
  auto img_view = gil::view(img);

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

  data = contig2dArray(width, height);
  procdata = contig2dArray(width, height);

  //Create scattering array
  if (rank == 0){
    for(int i = 0; i < size; ++i)
    {
      //Displacement from the start of the array
      disp[i] = i;
      //number of rows to send
      scatter_array[i] = height / size;
      printf("sca: %d, disp: %d\n", scatter_array[i], disp[i]);
    }
  }
  printf("Scattering... rank:%d\n",rank);
  // MPI_Scatter(&data[0][0], width * height, MPI_FLOAT,     //Sender info
  //             &procdata[0][0], width * height, MPI_FLOAT, //Recv info
  //             0, MPI_COMM_WORLD);                  //root and comm

  //Create the vector to skip rows
  // int MPI_Type_vector(int count, int blocklength, int stride,
  //                     MPI_Datatype oldtype, MPI_Datatype *newtype)

  MPI_Type_vector(1, width, size, MPI_FLOAT, &jump);
  MPI_Type_commit(&jump);
  // int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs, MPI_Datatype sendtype,
  //                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
  //                  int root, MPI_Comm comm)
  MPI_Scatterv(&(data[0][0]), scatter_array, disp, jump,
              &(procdata[0][0]), height / size, MPI_FLOAT,
              0, MPI_COMM_WORLD);

  //Each rank does its calculations
  printf("Calculating...rank:%d\n",rank);
  y = minY + (it * double(rank));
  for(int i = 0; i < height / size; i += 0)
  {
    x = minX;
    for (int j = 0; j < width; ++j)
    {
      procdata[j][i] = mandelbrot(x, y) / 512.0;
      x += jt;
    }
    y += (it * double(size));
    // curr_row = i*size;
  }
  printf("Gathering...\n");
  // MPI_Gather(&(procdata[0][0]), width * height, MPI_FLOAT,  //Recv info
  //           &(data[0][0]), width * height, MPI_FLOAT,  //Proc send info
  //           0, MPI_COMM_WORLD);                         //root info
  MPI_Gatherv(&(procdata[0][0]), height / size, MPI_FLOAT,
              &(data[0][0]), scatter_array, disp, jump,
              0, MPI_COMM_WORLD);
  if (rank == 0)
  {
    //Master renders image
    for (int i = 0; i < height; ++i)
    {
      for (int j = 0; j < width; ++j)
      {
        printf("i:%d, j:%d\n",i, j);
        img_view(j, i) = render(data[j][i]);
      }
    }
  }

    // int curr_row = 0;
    // for (int i = 0; 0 + (i * size) < height; ++i)
    // {
    //   for (int j = 0; j < width; ++j)
    //   {
    //     img_view(j, curr_row) = render(tempdata[j][i]);
    //   }
    //   curr_row += size;
    // }
    //Recieve all of the data from all of the processes
    // while (count < size - 1){
    //   //height / size will be the max height array we will recieve
    //   printf("waiting for recv\n");
    //   MPI_Recv(&(tempdata[0][0]), width * (height / size),
    //     MPI_FLOAT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
    //   printf("recieved\n");
    //
    //   int curr_row = status.MPI_SOURCE;

    //   ++count;
    // }
    free(procdata[0]);
    free(procdata);
    free(data[0]);
    free(data);

    gil::png_write_view("mandelbrot-test.png", const_view(img));


  // else
  // {
  //   float **data;
  //   data = contig2dArray(width, height / size);
  //
  //   printf("Calculating...\n");
  //   y = minY + (it * double(rank));
  //   for(int i = rank; rank + (i * size) < height; ++i)
  //   {
  //     x = minX;
  //     for (int j = 0; j < width; ++j)
  //     {
  //       data[j][i] = mandelbrot(x, y) / 512.0;
  //       x += jt;
  //     }
  //     y += (it * double(size));
  //   }
  //   printf("Sending data...%d\n", (width * ((height - rank) / size)));
  //   //Send data
  //   MPI_Send(&(data[0][0]), (width * ((height - rank) / size))
  //   , MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
  //   //free
  //   free(data[0]);
  //   free(data);
  // }
  MPI_Finalize();
  return 0;
}

/* eof */
