#include <pthread.h> 
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h> 

typedef enum{
    STATE_WAIT,
    STATE_MULT,
    STATE_ADD,
    STATE_PRINT,
} State;

typedef struct{
    State state;
    int * pendingMult;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    size_t nbIterations;
    size_t size;
    double * v1;
    double * v2;
    double * v3;
    double result;
} Product;

/*********** Data structure ***********/
Product prod;

/*********** Functions ***********/

void initPendingMult(Product * prod)
{
    size_t i;
    for(i=0;i<prod->size;i++)
    {
        prod->pendingMult[i]=1;
    }
}

int nbPendingMult(Product * prod)
{
    size_t i;
    int nb=0;
    for(i=0;i<prod->size;i++)
    {
        nb+=prod->pendingMult[i];
    }
    return(nb);
}

void wasteTime(unsigned long ms)
{
    unsigned long t,t0;
    struct timeval tv;
    gettimeofday(&tv,(struct timezone *)0);
    t0=tv.tv_sec*1000LU+tv.tv_usec/1000LU;
    do
    {
        gettimeofday(&tv,(struct timezone *)0);
        t=tv.tv_sec*1000LU+tv.tv_usec/1000LU;
    } 
    while(t-t0<ms);
}
/*****************************************************************************/
void * mult(void * data)
{
    size_t index;
    size_t iter;
    /*=>Recuperation de l’index, c’est a dire index = ... */
    index= *(size_t*) data;
    fprintf(stdout,"Begin mult(%ld)\n",index);
    /* Tant que toutes les iterations */
    for(iter=0;iter<prod.nbIterations;iter++) /* n’ont pas eu lieu */
        {

        /*=>Attendre l’autorisation de multiplication POUR UNE NOUVELLE ITERATION...*/

        /*verrou*/
        pthread_mutex_lock(&prod.mutex);

        while( STATE_MULT!=prod.state || prod.pendingMult[index]!=1) {
        pthread_cond_wait(&prod.cond,&prod.mutex);
        }

        pthread_mutex_unlock(&prod.mutex);
        /* fin du verrou*/

        fprintf(stdout,"--> mult(%ld)\n",index); /* La multiplication peut commencer */

        /*=>Effectuer la multiplication a l’index du thread courant... */
        prod.v3[index]=prod.v1[index]*prod.v2[index];

        wasteTime(200+(rand()%200)); /* Perte du temps avec wasteTime() */
        fprintf(stdout,"<-- mult(%ld) : %.3g*%.3g=%.3g\n", /* Affichage du */
        index,prod.v1[index],prod.v2[index],prod.v3[index]);/* calcul sur */
        /* l’index */

        /*=>Marquer la fin de la multiplication en cours... */
        prod.pendingMult[index]=0;
        /*=>Si c’est la derniere... */
        if ( 0==nbPendingMult(&prod) ){
            /*=>Autoriser le demarrage de l’addition... */

            /*verrou*/
            pthread_mutex_lock(&prod.mutex);
            prod.state= STATE_ADD;
            pthread_mutex_unlock(&prod.mutex);
            /* fin du verrou*/
            pthread_cond_broadcast(&prod.cond);

        }
    }
    fprintf(stdout,"Quit mult(%ld)\n",index);
    return(data);
}

/*****************************************************************************/

void * add(void * data)

    {
    size_t iter;
    fprintf(stdout,"Begin add()\n");
    /* Tant que toutes les iterations */
    for(iter=0;iter<prod.nbIterations;iter++) /* n’ont pas eu lieu */
    {
        size_t index;
        /*=>Attendre l’autorisation d’addition... */

        /*verrou*/
        pthread_mutex_lock(&prod.mutex);

        while ( !(STATE_ADD==prod.state) ){
        pthread_cond_wait(&prod.cond,&prod.mutex);
        }

        pthread_mutex_unlock(&prod.mutex);
        /* fin du verrou*/

        fprintf(stdout,"--> add\n"); /* l’addition peut commencer */
        /* Effectuer l’addition... */
        prod.result=0.0;
        for(index=0;index<prod.size;index++)
        {
            prod.result+= prod.v3[index];
        }
        wasteTime(100+(rand()%100)); /* Perdre du temps avec wasteTime() */
        fprintf(stdout,"<-- add\n");
        /*=>Autoriser le demarrage de l’affichage... */
        /*verrou*/
        pthread_mutex_lock(&prod.mutex);
        prod.state= STATE_PRINT;
        pthread_mutex_unlock(&prod.mutex);
        /* fin du verrou*/
        pthread_cond_broadcast(&prod.cond);

    }
    fprintf(stdout,"Quit add()\n");
    return(data);
}



/*****************************************************************************/
int main(int argc,char ** argv)
{

    size_t i, iter;
    pthread_t *multTh;
    size_t *multData;
    pthread_t addTh;
    void *threadReturnValue;

    /* A cause de warnings lorsque le code n’est pas encore la...*/
    (void)addTh; (void)threadReturnValue;
    /* Lire le nombre d’iterations et la taille des vecteurs */

    if((argc<=2)||
    (sscanf(argv[1],"%lu",&prod.nbIterations)!=1)||
    (sscanf(argv[2],"%lu",&prod.size)!=1)||
    ((int)prod.nbIterations<=0)||((int)prod.size<=0))
    {
        fprintf(stdout,"usage: %s nbIterations vectorSize\n",argv[0]);
        return(EXIT_FAILURE);
    }

    /* Initialisations (Product, tableaux, generateur aleatoire,etc) */
    
    prod.state=STATE_WAIT;
    prod.pendingMult=(int *)malloc(prod.size*sizeof(int));

    /*=>initialiser prod.mutex ... */
    pthread_mutex_init(&prod.mutex,NULL);
    /*=>initialiser prod.cond ... */
    pthread_cond_init(&prod.cond,NULL);

    /* Allocation dynamique des 3 vecteurs v1, v2, v3 */
    prod.v1=(double *)malloc(prod.size*sizeof(double));
    prod.v2=(double *)malloc(prod.size*sizeof(double));
    prod.v3=(double *)malloc(prod.size*sizeof(double));
    /* Allocation dynamique du tableau pour les threads multiplieurs */
    multTh=(pthread_t *)malloc(prod.size*sizeof(pthread_t));
    /* Allocation dynamique du tableau des MulData */
    multData=(size_t *)malloc(prod.size*sizeof(size_t));
    /* Initialisation du tableau des MulData */
    for(i=0;i<prod.size;i++)
    {
        multData[i]=i;
    }

    /*=>Creer les threads de multiplication... */ //on crée N threads pour la multiplication
    for(int k=0; k< prod.size ; k++) {

        if ( pthread_create(&multTh[k], NULL, mult, &multData[k]) != 0 ) {
            perror("thread mult creation error");
            return EXIT_FAILURE;
        }

    }

    /*=>Creer le thread d’addition... */
    if (pthread_create(&addTh, NULL, add, NULL) != 0) {
        perror("thread add creation error");
        return EXIT_FAILURE;
    }

    srand(time((time_t *)0)); /* Init du generateur de nombres aleatoires */

    /* Pour chacune des iterations a realiser, c’est a dire : */
    for(iter=0;iter<prod.nbIterations;iter++) /* tant que toutes les iterations */
    { /* n’ont pas eu lieu */

        size_t j;

        /* Initialiser aleatoirement les deux vecteurs */
        for(j=0;j<prod.size;j++)
        {
            prod.v1[j]=10.0*(0.5-((double)rand())/((double)RAND_MAX));
            prod.v2[j]=10.0*(0.5-((double)rand())/((double)RAND_MAX));
        }

        fprintf(stdout,"v1 is: ");
        for(int l=0; l<prod.size;l++){
            fprintf(stdout,"%lf,",prod.v1[l]);
        }
        fprintf(stdout,"\nV2 is: ");
        for(int l=0; l<prod.size;l++){
            fprintf(stdout,"%lf,",prod.v2[l]);
        }
        fprintf(stdout,"\n");

        /*=>Autoriser le demarrage des multiplications pour une nouvelle iteration..*/

        initPendingMult(&prod);
        /*verrou*/
        pthread_mutex_lock(&prod.mutex);
        prod.state = STATE_MULT;
        pthread_mutex_unlock(&prod.mutex);
        /* fin du verrou*/
        pthread_cond_broadcast(&prod.cond);


        /*=>Attendre l’autorisation d’affichage...*/
        //while(STATE_PRINT!=prod.state);

        /*verrou*/
        pthread_mutex_lock(&prod.mutex);

        while( !(STATE_PRINT==prod.state) ){
        pthread_cond_wait(&prod.cond,&prod.mutex);
        }

        pthread_mutex_unlock(&prod.mutex);
        /* fin du verrou*/

        /*=>Afficher le resultat de l’iteration courante...*/
        printf("current iteration result is %lf \n",prod.result);
        fprintf(stdout,"\n\n");

    }

    /*=>Attendre la fin des threads de multiplication...*/
    for(int k=0; k<prod.size; k++){
        pthread_join(multTh[k],NULL);
    }

    /*=>Attendre la fin du thread d’addition...*/
    pthread_join(addTh,NULL);

    /*=> detruire prod.cond ... */
    pthread_cond_destroy(&prod.cond);
    /*=> detruire prod.mutex ... */
    pthread_mutex_destroy(&prod.mutex);
    
    /* Detruire avec free ce qui a ete initialise avec malloc */
    free(prod.pendingMult);
    free(prod.v1);
    free(prod.v2);
    free(prod.v3);
    free(multTh);
    free(multData);
    return(EXIT_SUCCESS);
}