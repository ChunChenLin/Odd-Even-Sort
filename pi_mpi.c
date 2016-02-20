#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <math.h>

int main(int argc, char *argv[]){
	int rank, size;
        int i;
        double x=0.0,y=0.0;
        double area=0.0,sum=0.0;
        double pieces;

        size = atoi(argv[1]);
        pieces = atoi(argv[2]);        

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
        //divide to  pieces
	for(i = 0; i < pieces/size; i ++){
	      //x = (1.0)/(1000000)*(rank*1000000/size+i);
	      //x = i * ( (1.0) / 1000000 ) + (rank / size);
	      // x=(1.0)/(1000000)*(rank*1000000/size+i);
	      x = (1.0/pieces)*i + (1.0/pieces)*rank*pieces/size;   
              y = sqrt(1-x*x);
              area += y * (1.0/pieces);
	}
	MPI_Reduce(&area, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	if(rank == 0) printf("pi = %f\n", 4*sum);	

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}
