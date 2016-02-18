/**
 *  \file mandelbrot_ms.cc
 *
 *  \brief Implement your parallel mandelbrot set in this file.
 */
#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include "render.hh"

#include <time.h>

using namespace std;

#define WIDTH 1000
#define HEIGHT 1000

#define REQUEST 1
#define DATA 0
//TRY CALLOC
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

   if (size < 2) return 0;

   //Broadcast all nodes start range
   if (rank == 0)
   {
     //Master
     MPI_Status status;
     int count;
    int *chunk_details;
    chunk_details = (int*)malloc(sizeof(int)*2);
     //alloc image array
     gil::rgb8_image_t img(height, width);
     auto img_view = gil::view(img);

     //data array for the temp chunk
     float **tempdata;
     float **data;
     data = contig2dArray(width, height);
     int *temp; //junk variable

     //set chunk size
     chunk_details[0] = 500;
     //set chunk row start
     chunk_details[1] = 0;

     while (chunk_details[1] + chunk_details[0] <= height)
     {
       printf("Waiting for request\n");
       MPI_Recv(&temp, 1,
         MPI_INT, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &status);
              //send the chunk details out
       printf("Sending next chunk...\n");
       MPI_Send(&(chunk_details[0]), 2, MPI_INT, status.MPI_SOURCE, DATA, MPI_COMM_WORLD);
       tempdata = contig2dArray(width, chunk_details[0]);

       printf("Waiting for chunk...\n");
       MPI_Recv(&(tempdata[0][0]), width * chunk_details[0], MPI_FLOAT,
                status.MPI_SOURCE, DATA, MPI_COMM_WORLD, &status);
       for (int i = 0; i < chunk_details[0]; ++i)
       {
        for(int j = 0; j < width; ++j)
        {
          data[j][chunk_details[1] + i] = tempdata[j][i];
        }
       }
       //increment start row
       //break;
       chunk_details[1] += (chunk_details[0] - 1);
     //   return 1;
     }
     printf("Listening for final request...\n");
     MPI_Recv(&temp, 1,
       MPI_INT, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &status);
     chunk_details[0] = -1;
     chunk_details[1] = -1;
     printf("Sending end.\n");
     MPI_Send(&(chunk_details[0]), 2, MPI_INT, 1, DATA, MPI_COMM_WORLD);
     printf("Sent. Rendering...\n");

     //render the image
     for (int i = 0; i < height; ++i)
     {
       for (int j = 0; j < width; ++j)
       {
         img_view(j, i) = render(data[j][i]);
       }
     }
     free(tempdata[0]);
     free(tempdata);
     free(chunk_details);

     gil::png_write_view("mandelbrot-test-ms.png", const_view(img));

   }
   else
   {
      float **data;
      int *junk;
      int *chunk_details;
      chunk_details = (int*)malloc(sizeof(int)*2);
      MPI_Status status;
      int done = 0;

     while (done == 0)
     {
       printf("Sending Request.\n");
       MPI_Send(&junk, 1, MPI_INT, 0, REQUEST, MPI_COMM_WORLD);
       printf("Waiting for chunk_details.\n");
       MPI_Recv(&(chunk_details[0]), 2, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);
       printf("details[0]: %d  details[1]: %d\n",chunk_details[0], chunk_details[1]);
       //Determine if were done or not
       if (chunk_details[0] == -1 && chunk_details[1] == -1)
       {
         done = 1;
       }
       else
       {
         data = contig2dArray(width, chunk_details[0]);

         printf("Calculating row:%d size: %d\n", chunk_details[1], chunk_details[0]);
         y = minY + (it * double(chunk_details[1]));
         for(int i = 0; i < chunk_details[0]; ++i)
         {
           x = minX;
           for (int j = 0; j < width; ++j)
           {
             data[j][i] = mandelbrot(x, y) / 512.0;
             x += jt;
           }
           y += it;
         }
         printf("Sending data...\n");
         //Send data
         MPI_Send(&(data[0][0]), (width * chunk_details[0]), MPI_FLOAT, 0, DATA, MPI_COMM_WORLD);
         free(data[0]);
         free(data);
       }
     }
     //free
     free(chunk_details);
   }
   MPI_Finalize();
   return 0;
 }

 /* eof */
