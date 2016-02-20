#include<stdio.h>
#include<string>
#include<stdlib.h>
#include<mpi.h>
#include<algorithm>
//#include<ctime>

using namespace std;

int rnk, size;
int n, length;
int *buffer;

double tIO,tTMP,tSTART,tEND,tCOMM;

inline int L(int r){
    return r*length + 1;
}
inline int R(int r){
    return r==size-1 ? n : (r+1)*length;
} 
/*                                                                   //  p----------------> q----------->
inline void merge(int left, int middle, int right){                  //  --------------------------------
    int p=left, q=middle+1;                                          //  |left       mid|dle       right|
    int *tmp = new int[n+1], index=left;                             //  | <buffer>     |               |
    while(p <= middle && q <= right){                                //  --------------------------------
        if(buffer[p] < buffer[q]) tmp[index ++] = buffer[p ++];
        else tmp[index ++] = buffer[q ++];
    }
    while(p <= middle) tmp[index ++] = buffer[p ++]; //compensate
    while(q <= right) tmp[index ++] = buffer[q ++];
    for(int i=left; i <= right; i ++) buffer[i] = tmp[i];
    free(tmp);
}

*/
int main(int argc, char* argv[]){
    n = atoi(argv[1]);
    buffer = new int[n+1];
    
    MPI_Status status;
    MPI_File in, out;
    
    MPI_Init(&argc, &argv);
    
    tSTART = MPI_Wtime();
    
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rnk);
    
    MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &in);
    MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &out);

    /* just sort once by rank0 */
    if(n <= size || size == 1) {
        int left=1; 
        int right = (rnk == 0 ? n : 0);
        
        tTMP = MPI_Wtime();                                              /*  store datas from buffer[1] */
        MPI_File_read_at_all(in, (left-1)*sizeof(int), buffer+left, right-left+1, MPI_INT, &status);
        MPI_File_close(&in);
        tIO += MPI_Wtime()-tTMP; //Input

        if(rnk == 0) sort(buffer+1, buffer+1+n);            
        MPI_Barrier(MPI_COMM_WORLD);
        
        tTMP = MPI_Wtime(); 
        MPI_File_write_at_all(out, (left-1)*sizeof(int), buffer+left, right-left+1, MPI_INT, &status);
        MPI_File_close(&out);
        tIO += MPI_Wtime()-tTMP; //Output
        
        if(rnk == 0){
            tEND = MPI_Wtime();
            printf("Total: %f\n", tEND-tSTART);
            printf("IO: %f\n", tIO);
            printf("COMM: %f\n", tCOMM);
            printf("CPU: %f\n", tEND-tSTART-tIO-tCOMM);
        }

        MPI_Finalize();
        return 0;
    }
    
    length = n/size;
    int left=L(rnk), right=R(rnk);
    
    tTMP = MPI_Wtime();
    MPI_File_read_at_all(in, (left-1)*sizeof(int), buffer+left, right-left+1, MPI_INT, &status);
    MPI_File_close(&in);
    tIO += MPI_Wtime()-tTMP; //Input

    sort(buffer+L(rnk), buffer+R(rnk)+1); // every process sorts it' s allocated buffer first
    
    //int global_not_sorted=1;
    int OddToEven = 1;
    while(1) {
        if(OddToEven == 1) { // 3-->2-->3      5-->4-->5
            if(rnk%2==0 && rnk+1<size) {
                tTMP = MPI_Wtime();
                MPI_Recv(&buffer[L(rnk+1)], R(rnk+1)-L(rnk+1)+1, MPI_INT, rnk+1, rnk, MPI_COMM_WORLD, &status);
                tCOMM += MPI_Wtime()-tTMP;
            }
            else if(rnk%2==1 && rnk<size) {
                tTMP = MPI_Wtime();
                MPI_Send(&buffer[L(rnk)], R(rnk)-L(rnk)+1, MPI_INT, rnk-1, rnk-1, MPI_COMM_WORLD);
                tCOMM += MPI_Wtime()-tTMP;
            }

            if(rnk%2==0 && rnk+1<size) {
                sort(buffer+L(rnk), buffer+R(rnk+1)+1);
                /* After recv, sort it and then send the result back */
                tTMP = MPI_Wtime();
                MPI_Send(&buffer[L(rnk+1)], R(rnk+1)-L(rnk+1)+1, MPI_INT, rnk+1, rnk, MPI_COMM_WORLD);
                tCOMM += MPI_Wtime()-tTMP;

            } else if(rnk%2==1 && rnk<size)  {
                /* Recv what sorted by the prev one*/
                tTMP = MPI_Wtime(); 
                MPI_Recv(&buffer[L(rnk)], R(rnk)-L(rnk)+1, MPI_INT, rnk-1, rnk-1, MPI_COMM_WORLD, &status);
                tCOMM += MPI_Wtime()-tTMP;
            }
        } else { // 2-->1-->2  4-->3-->4
            if(rnk%2==0 && rnk-1>0)  {
                tTMP = MPI_Wtime();
                MPI_Send(&buffer[L(rnk)], R(rnk)-L(rnk)+1, MPI_INT, rnk-1, rnk-1, MPI_COMM_WORLD);
                tCOMM += MPI_Wtime()-tTMP;
            }
            else if(rnk%2==1 && rnk+1<size) {
                tTMP = MPI_Wtime();
                MPI_Recv(&buffer[L(rnk+1)], R(rnk+1)-L(rnk)+1, MPI_INT, rnk+1, rnk, MPI_COMM_WORLD, &status);
                tCOMM += MPI_Wtime()-tTMP;
            }
            
            if(rnk%2==0 && rnk-1>0) {
                tTMP = MPI_Wtime();
                MPI_Recv(&buffer[L(rnk)], R(rnk)-L(rnk)+1, MPI_INT, rnk-1, rnk-1, MPI_COMM_WORLD, &status);
                tCOMM += MPI_Wtime()-tTMP;
            } else if(rnk%2==1 && rnk+1<size) { 
                //merge(L(rnk), R(rnk), R(rnk+1)); //!!!!!!
                sort(buffer+L(rnk), buffer+R(rnk+1)+1);
                tTMP = MPI_Wtime();
                MPI_Send(&buffer[L(rnk+1)], R(rnk+1)-L(rnk+1)+1, MPI_INT, rnk+1, rnk, MPI_COMM_WORLD);
                tCOMM += MPI_Wtime()-tTMP;
            }
        }
        
        /* copy the following to compare*/
        if(rnk == size-1) {
            tTMP = MPI_Wtime();
            MPI_Send(&buffer[L(rnk)], n-L(rnk)+1, MPI_INT, rnk-1, rnk, MPI_COMM_WORLD);
            tCOMM += MPI_Wtime()-tTMP;
        }
        else if(rnk == 0) {
            tTMP = MPI_Wtime();
            MPI_Recv(&buffer[L(rnk+1)], n-L(rnk+1)+1, MPI_INT, rnk+1, rnk+1, MPI_COMM_WORLD, &status);
            tCOMM += MPI_Wtime()-tTMP;
        }
        else {
            /* Recv from tail*/
            tTMP = MPI_Wtime();
            MPI_Recv(&buffer[L(rnk+1)], n-L(rnk+1)+1, MPI_INT, rnk+1, rnk+1, MPI_COMM_WORLD, &status);
            /* Send to head */
            MPI_Send(&buffer[L(rnk)], n-L(rnk)+1, MPI_INT, rnk-1, rnk, MPI_COMM_WORLD);
            tCOMM += MPI_Wtime()-tTMP;
        }
        
        
        int is_sorted = 1;
        if(rnk == 0) {
            is_sorted = 1;
            for(int i=1;i<=n-1;i++) {
                if(buffer[i]>buffer[i+1]){
                    is_sorted = 0;
                    break;
                }
            }
            tTMP = MPI_Wtime();
            MPI_Send(&is_sorted, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); //send to next
            tCOMM += MPI_Wtime()-tTMP;   
        } 
        else if(rnk == size-1) {
            tTMP = MPI_Wtime();
            MPI_Recv(&is_sorted, 1, MPI_INT, rnk-1, rnk-1, MPI_COMM_WORLD, &status);
            tCOMM += MPI_Wtime()-tTMP;
        }
        else {
            tTMP = MPI_Wtime();
            MPI_Recv(&is_sorted, 1, MPI_INT, rnk-1, rnk-1, MPI_COMM_WORLD, &status);
            MPI_Send(&is_sorted, 1, MPI_INT, rnk+1, rnk, MPI_COMM_WORLD);
            tCOMM += MPI_Wtime()-tTMP;
        }

        if(is_sorted == 1) break;
        OddToEven = (OddToEven+1)%2;
    }
    
    tTMP = MPI_Wtime();
    MPI_File_write_at_all(out, (left-1)*sizeof(int), buffer+left, right-left+1, MPI_INT, &status);
    MPI_File_close(&out);
    tIO += MPI_Wtime()-tTMP; //Output

    MPI_Barrier(MPI_COMM_WORLD);
    
    if(rnk == 0){
        tEND = MPI_Wtime();
        printf("Total: %f\n", tEND-tSTART);
        printf("IO: %f\n", tIO);
        printf("COMM: %f\n", tCOMM);
        printf("CPU: %f\n", tEND-tSTART-tIO-tCOMM);
    }
    
    MPI_Finalize();

    return 0;
}

