#include<stdio.h>
#include<string.h>
#include<math.h>
#include<pthread.h>

int size,pieces;
double tmp[1000];
double x,y;

void *run(void *param)
{
    int id = *(int*)param;
    int i;
    for(i=0;i<pieces/size;i++) {
	x = i*(1.0)/pieces + id*(1.0)/pieces*pieces/size;
        y = sqrt(1-x*x);
        tmp[id] += y*(1.0)/pieces;
    }
    pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
    size = atoi(argv[1]);
    pieces = atoi(argv[2]);
   
    pthread_t thread_id[size];
    pthread_attr_t my_attr;

    int i;
    int arr[size];
    for(i=0;i<size;i++) {
        pthread_attr_init(&my_attr);
        arr[i] = i;
        tmp[i] = 0;
        pthread_create(&thread_id[i],&my_attr,run,(void*)&arr[i]);
    }
    
    double ans = 0.0;
    for(i=0;i<size;i++) {
	pthread_join(thread_id[i],NULL);
        ans += tmp[i];
    }

    printf("pi = %f\n",4*ans);
    return 0;
}
