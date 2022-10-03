#include <pthread.h> 
#include <stdio.h> 
#include <stdlib.h> 


static long int *ptr;

struct myargs{
    int input;
    long int output;
} myargs_t;



void *func(void *arg) {

    ptr=malloc(sizeof(long int));

    long int fact=1;
    int currentFact=0;
    int maxFact=(int)(((struct myargs *) arg)->input);

    ((struct myargs *) arg)->output=1;

    while(currentFact<maxFact){
        currentFact+=1;
        fact=fact*currentFact;
        printf("running %ld\n",pthread_self());
    }

    printf("factorial value is %ld\n",fact);
    
    ((struct myargs *) arg)->output=fact;
    *ptr=fact;
    
    pthread_exit(NULL);
    return NULL;
}


int main(int argc, char *argv[]) {

    pthread_t th1;
    pthread_t th2;
 
    struct myargs a;
    struct myargs b;

    a.input=6;
    b.input=3;

    if (pthread_create(&th1, NULL, func, &a) != 0) {
        perror("thread 1 creation error");
        return EXIT_FAILURE;
    }
    if (pthread_create(&th2, NULL, func, &b) != 0) {
        perror("thread 2 creation error");
        return EXIT_FAILURE;
    }

    pthread_join(th1,NULL);
    pthread_join(th2,NULL);

    printf("%ld\n",*ptr);
    free(ptr);

    return EXIT_SUCCESS;
} 