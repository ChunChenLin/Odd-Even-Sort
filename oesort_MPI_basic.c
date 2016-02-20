#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mpi.h>

double tIO,tTMP,tSTART,tEND,tCOMM;

int main(int argc, char **argv)
{
    int rank,size;
    int i,n,length;
    int start,end;
    int tail=0;
    int *buffer;
    int sbuffer[1];
    int rbuffer[1];
    //int Stmp,Rtmp;
    n = atoi(argv[1]); //size of list

    MPI_Status    status;
    MPI_File      fh;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    
    tSTART = MPI_Wtime();
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    /* Open file to read */
    MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    /* calculate the range for each process to read */
    size = (n<size) ? n : size;

    length = n / size;
    start = length * rank;
    if (rank == size-1) {
        end = n;
    }else {
       end = start + length;
    }

    /*border: 0~tail for buffer of each process*/
    tail = end - start - 1;

    //printf("rank%d : length%d start%d\n",rank,length,start);

    /* Allocate space */
    buffer = (int *)malloc((end - start) * sizeof(int));
    //rbuffer = (int *)malloc((end - start+1) * sizeof(int));
    for(i=0; i<=tail; i++) {
        buffer[i] = 0;
    }

    /* Each process read in data from the file */
    if(rank>=n) {
        tTMP = MPI_Wtime();
        MPI_File_seek(fh, start, MPI_SEEK_SET);
        MPI_File_read_at_all(fh, 0, buffer, 0, MPI_INT, &status);
        tIO += MPI_Wtime()-tTMP;
    } else {
        tTMP = MPI_Wtime();
        MPI_File_seek(fh, start, MPI_SEEK_SET);
        MPI_File_read_at_all(fh, start*sizeof(int), buffer, end-start, MPI_INT, &status);
        tIO += MPI_Wtime()-tTMP;
    }

    /*printf("rank%d :",rank);
    for(i=0;i<=tail;i++) printf("%d ",buffer[i]);
    printf("\n");*/
    MPI_Barrier(MPI_COMM_WORLD);
    /* close the file */
    MPI_File_close(&fh);

    int tmp;
    int notDone=1;
    int global_notDone=1;
    int Continue = 1; // communicate
    int global_Continue = 1;
    int tag = 100;
    
    
    while(global_notDone || global_Continue) {    
    
        sbuffer[0]=0;
        rbuffer[0]=0;
        notDone = 0;
        Continue = 0;
        global_Continue = 1;

        /*printf("rank%d :",rank);
        for(i=0; i<=tail; i++) {
            printf("%d ",buffer[i]);
        }
        printf("\n");*/

        if(rank%2==0 && rank<n) {
            //odd-even
            for(i=1; i<=tail-1; i+=2) {
                if(buffer[i] > buffer[i+1]) {
                    tmp = buffer[i];
                    buffer[i] = buffer[i+1];
                    buffer[i+1] = tmp;
                    notDone = 1;
                    //printf("rank%d A\n\n",rank);
                }
            }
            //rank N 's head communicates with rank (N-1) 's tail (A)
            if(rank!=0 && global_Continue==1) {
                //printf("rank N 's head communicates with rank (N-1) 's tail\n");
                sbuffer[0] = buffer[0]; //head
                tTMP = MPI_Wtime();
                MPI_Send(sbuffer,1,MPI_INT,rank-1,tag*rank,MPI_COMM_WORLD);
                //printf("rank%d send %d to rank%d successfully\n",rank,sbuffer[0],rank-1);
                MPI_Recv(rbuffer,1,MPI_INT,rank-1,tag*(rank-1),MPI_COMM_WORLD,&status);
                //printf("rank%d recv %d from rank%d successfully\n",rank,rbuffer[0],rank-1);
                tCOMM += MPI_Wtime()-tTMP;
                if(buffer[0] < rbuffer[0]) {
                    buffer[0] = rbuffer[0];
                    Continue = 1;
                }
                //MPI_Barrier(MPI_COMM_WORLD);
            }
            //even-odd
            for(i=0; i<tail; i+=2) {
                //if(i+1<=tail) { //avoid exceeding array 's  range (buffer[i+1])
                    if(buffer[i] > buffer[i+1]) {
                        tmp = buffer[i];
                        buffer[i] = buffer[i+1];
                        buffer[i+1] = tmp;
                        notDone = 1;
                        //printf("rank%d B\n\n",rank);
                    }
                //}
            }
            //rank N 's tail communicates with rank (N+1) 's head (B)
            if(rank!=size-1 && global_Continue==1) { 
                sbuffer[0] = buffer[tail]; //tail
                tTMP = MPI_Wtime();
                MPI_Send(sbuffer,1,MPI_INT,rank+1,tag*rank,MPI_COMM_WORLD);
                //printf("rank%d send %d to rank%d successfully\n",rank,sbuffer[0],rank+1);
                MPI_Recv(rbuffer,1,MPI_INT,rank+1,tag*(rank+1),MPI_COMM_WORLD,&status);
                //printf("rank%d recv %d from rank%d successfully\n",rank,rbuffer[0],rank+1);
                tCOMM += MPI_Wtime()-tTMP;
                if(buffer[tail] > rbuffer[0]) {
                    buffer[tail] = rbuffer[0];
                    Continue = 1;
                }
            }
        } else if(rank%2!=0 && rank<n) {
            //odd-even
            for(i=0; i<tail; i+=2) {
                //if(i+1<=tail) { //avoid exceeding array 's  range (buffer[i+1])
                    if(buffer[i] > buffer[i+1]) {
                        tmp = buffer[i];
                        buffer[i] = buffer[i+1];
                        buffer[i+1] = tmp;
                        notDone = 1;
                        //printf("rank%d C\n\n",rank);
                    }
                //}
            }
            if(rank!=size-1 && global_Continue==1) { //rank N 's tail communicates with rank (N+1) 's head (A)
                //printf("rank N 's tail communicates with rank (N+1) 's head\n");
                sbuffer[0] = buffer[tail]; //tail
                tTMP = MPI_Wtime();
                MPI_Send(sbuffer,1,MPI_INT,rank+1,tag*rank,MPI_COMM_WORLD);
                //printf("rank%d send %d to rank%d successfully\n",rank,sbuffer[0],rank+1);
                MPI_Recv(rbuffer,1,MPI_INT,rank+1,tag*(rank+1),MPI_COMM_WORLD,&status);
                //printf("rank%d recv %d from rank%d successfully\n",rank,rbuffer[0],rank+1);
                tCOMM += MPI_Wtime()-tTMP;
                if(buffer[tail] > rbuffer[0]) {
                    buffer[tail] = rbuffer[0];
                    Continue = 1;
                }
                //MPI_Barrier(MPI_COMM_WORLD);
            }
            //even-odd
            for(i=1; i<=tail-1; i+=2) {
                if(buffer[i] > buffer[i+1]) {
                    tmp = buffer[i];
                    buffer[i] = buffer[i+1];
                    buffer[i+1] = tmp;
                    notDone = 1;
                    //printf("rank%d D\n\n",rank);
                }
            }
            if(rank!=0 && global_Continue==1) { //rank N 's head communicates with rank (N-1) 's tail (B)
                sbuffer[0] = buffer[0]; //head
                tTMP = MPI_Wtime();
                MPI_Send(sbuffer,1,MPI_INT,rank-1,tag*rank,MPI_COMM_WORLD);
                //printf("rank%d send %d to rank%d successfully\n",rank,sbuffer[0],rank+1);
                MPI_Recv(rbuffer,1,MPI_INT,rank-1,tag*(rank-1),MPI_COMM_WORLD,&status);
                //printf("rank%d send %d to rank%d successfully\n",rank,sbuffer[0],rank+1);
                tCOMM += MPI_Wtime()-tTMP;
                if(buffer[0] < rbuffer[0]) {
                    buffer[0] = rbuffer[0];
                    Continue = 1;
                }
            }
        }
        
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Reduce(&notDone, &global_notDone, 1, MPI_INT, MPI_LOR, 0, MPI_COMM_WORLD);
        MPI_Bcast(&global_notDone, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Reduce(&Continue, &global_Continue, 1, MPI_INT, MPI_LOR, 0, MPI_COMM_WORLD);
        MPI_Bcast(&global_Continue, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        
        //printf("rank%d done=%d Continue=%d global_Continue=%d\n\n",rank,done,Continue,global_Continue);
        //printf("rank%d -------------------------------------------------------------\n\n",rank);

    }
    //printf("rank%d out of while\n",rank);
    MPI_Barrier(MPI_COMM_WORLD);

    //printf("rank%d out of while\n",rank);

    /* Open file to write */
    MPI_File_open(MPI_COMM_WORLD, argv[3],MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);

    //printf("rank%d writing\n",rank);
    tTMP = MPI_Wtime();
    if(rank>=n) MPI_File_write_at_all(fh, 0, buffer, 0, MPI_INT, &status);
    else MPI_File_write_at_all(fh, start*sizeof(int), buffer, end-start, MPI_INT, &status);
    tIO += MPI_Wtime()-tTMP;


    /* close the file */
    MPI_File_close(&fh);

    MPI_Barrier(MPI_COMM_WORLD);
    
    if(rank == 0){
        tEND = MPI_Wtime();
        printf("Total: %f\n", tEND-tSTART);
        printf("IO: %f\n", tIO);
        printf("COMM: %f\n", tCOMM);
        printf("CPU: %f\n", tEND-tSTART-tIO-tCOMM);
    }
    MPI_Finalize();

    return 0;
}
