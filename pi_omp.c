#include<stdio.h>
#include<math.h>
#include<omp.h>

double tmp[100];
int size,pieces;
double ans;

int main(int argc,char *argv[])
{
    //double x = 0.0,y = 0.0;

    size = atoi(argv[1]);
    pieces = atoi(argv[2]);

    int i;
    //int id = omp_get_thread_num();
    #pragma omp parallel num_threads(size)
    {    
         int id = omp_get_thread_num();
         double x=0.0,y=0.0;
         #pragma omp for  
              for(i=0;i<pieces;i++) {
                  x = (1.0/pieces)*i; 
                  y = sqrt(1.0-x*x);
                  tmp[id] += y*(1.0/pieces); 
              }
    }
    for(i=0;i<size;i++) ans += tmp[i];
    printf("pi = %f\n",4*ans);

    return 0;
}
