//GLOBAL:------------------
//Import libraries:
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

//Thread_Data Struct
typedef struct Data {
  int id;
  int start_x;
  int start_y;
  int ncells;
}Data;

//Function definitions--------
void print_matrices(float ** a1, float ** a2);
float ** init_matrix(float ** arr, int conf);
Data * getThreadData(int quo, int rem);
void *relax();

//Global variables
//ENV VARS: Maybe struct it
int n = 6;//Square matrix size
int nthreads = 2;
float precision = 0.1;
int outsideThreshold;

//Init Matrices
float **a1;
float **a2;
pthread_barrier_t barrier;

//Array of thread data structs
Data *threadDataArray;

//Precision array:
int parray;

int main(void) {

  //Create Data-----------
  a1 = init_matrix(a1, 1); //Create populated matrix
  a2 = init_matrix(a2, 0); //Create empty matrix with borders
  print_matrices(a1, a2);

  //Init Data Structures:
  threadDataArray = malloc(nthreads * sizeof(Data));
  parray = malloc(nthreads*(sizeof(int)));

  //Get remainder & quotient for coordinate calculations
  int ncompute = (n-2)*(n-2); //Number of computable cells
  int quo = ncompute/nthreads; //Number of cells per thread
  int rem = ncompute % nthreads; //Remainder after division

  //Calculate thread data
  threadDataArray = getThreadData(quo, rem);

  //Create Threads--------
  //Allocate memory for threads:
	pthread_t *threads = malloc(nthreads*sizeof(pthread_t));

  //Init Barriers-------
  pthread_barrier_init(&barrier, NULL, nthreads+1);

  for (int i=0; i < nthreads; i++){
	  pthread_create(&threads[i], NULL, relax, &threadDataArray[i]);
	}

  outsideThreshold = 0;

  //Loop until precision is reached
  while (1) {
    pthread_barrier_wait(&barrier);
      printf("\nPrecision array contents:\n");
    for (int c = 0; c < nthreads; c++) {
      printf("%d", parray[c]);
      printf("\n");
      if (!parray[c]) {
        //Global precision not met
        //Precision not reached
        pthread_barrier_wait(&barrier);
      } else{
        //Precision reached
        //Sync threads
        outsideThreshold = 1;
        printf("Global precision reached- BREAKING");
        pthread_barrier_wait(&barrier);
        break; //exit
      }
      printf("whiling");
    }
    //ONLY EXITS IF GLOBAL PRECISION MET
  }
  //Wait for workers to finish messing around
  for (int i=0; i < nthreads; i++){
	  pthread_join(threads[i], NULL);
	}
  //Free memory
  free(a1);
  free(a2);
  free(threadDataArray);
  free(parray);
  free(threads);

  printf("All is done");
  return 0;
}


//-------PROGRAM FUNCTIONS------

//RELAX FUNCTION
void *relax(void *arg) {
  Data *t_data = (Data *)arg;
  int sx = t_data->start_x;
  int sy = t_data->start_y;
  int c = t_data->ncells;
  int id = t_data->id;
  int i, j, t, p, count;

  float cross;
  while (1) {
    p = 1;
    j = sx;
    i = sy;
    printf("\nThread: %d:Starting at [%d,%d] for %d\n", id, i, j, c);
    for (count = 0; count < c; count++) {
      cross = a1[i][j-1] + a1[i][j+1] + a1[i-1][j] + a1[i+1][j];
      a2[i][j] =  (cross/4);

      if (a2[i][j] - a1[i][j] > precision) {
        //Precision not reached for any cell
        p = 0;
        printf("noo");
      }

      //Prepare for next iteration
      if (i == n-2) { //Onto next line: reset x & increment y
        i = 1;
        j++;
      } else { //Same line: increment x
        i++;
      }
    }

    parray[id] = p;
    printf("\n");
    printf("Precision returned:");
    printf("%d", p);
    printf("\n");

    for (int k = 0; k < nthreads; k++) {
      printf("\nPrecision Array for ID: %d\n", id);
      printf("%d",parray[k]);
    }


    //Barrier
    pthread_barrier_wait(&barrier);
    pthread_barrier_wait(&barrier);

    if (outsideThreshold) {
      printf("precision not reached, continuing");
    }
    else {
      //Overall precision has been met lets goooooo
      printf("SPAGHETTI MONSTER");
      break;
    }

    //End of iteration
    //Swap matrices
    float ** temp = a1;
    a1 = a2;
    a2 = temp;

    print_matrices(a1, a2);
    printf("LOCHNESS MONSTER\n");
  }
  pthread_exit(NULL);
  printf("Thread %d: Terminating\n", id);
}



//-------HELPER METHODS --------

Data *getThreadData(int quo, int rem){

  int rcount = 0;
  int ncount;
  int yn;
  int xn;

//Iterate through number of threads
  for (int i = 0; i<nthreads; i++) {
    //Create thread_data object from Data struct
    Data thread_data;
    if (i == 0){
      thread_data.start_y = 1;
      thread_data.start_x = 1;
      xn = 1;
      yn = 1;
    }
    else {
      if (rcount > rem) {
        yn = yn + quo;
      } else{
        yn = yn + quo + 1;
      }
      if (yn % (n-2) == 0){
        xn = (xn + floor(yn/(n-2))) -1;
        yn = (n-2);
      } else {
        xn = xn + floor(yn/(n-2));
        yn = yn % (n-2);
      }
    }
    rcount++;
    if (rcount > rem){
      thread_data.ncells = quo;
    }
    else {
      thread_data.ncells = quo +1;
    }
    thread_data.start_y = yn;
    thread_data.start_x = xn;
    thread_data.id = i;
    threadDataArray[i] = thread_data;
    //Print coordinates & cell numbers
    printf("%d", xn);
    printf("\t");
    printf("%d", yn);
    printf("\t");
    printf("%d", thread_data.ncells);
    printf("\n");
  }
  return threadDataArray;
}



float ** init_matrix(float **arr, int conf){

  //Init array of pointers to pointers
  int r = n, c = n, i, j, count;

  //Allocate memory for row pointers
  arr = malloc(r * sizeof(int *));
  for (i = 0; i < r; i++) {
    //Allocate memory for column values
    arr[i] = malloc(c * sizeof(float));
  }
  //Start integer count
  count = 0;
    //Loop through indices of ** array
    for (i = 0; i < r-0; i++){
        for (j = 0; j < c-0; j++){
            //Assign value as count
            if (conf) {
              arr[i][j] = ((double)rand()/(double)RAND_MAX);
            } else {
              arr[i][j] = 0;
            }
            //Configure border numbers: 
            //Zero on bottom and right axes
            if (i == r-1){
              arr[i][j] = 0;
            }
            if (j == c-1){
              arr[i][j] = 0;
            }
            //Ones on left and top axes
            if (i == 0) {
              arr[i][j] = 1;
            }
            if (j == 0) {
              arr[i][j] = 1;
            }
          }
         }
      return arr;
  }

//PRINT MATRIX
void print_matrices(float ** a1, float ** a2) {
  printf("----------------MATRIX 1----------------");
  printf("|");
  printf("\t");
  printf("|");
printf("----------------MATRIX 2----------------");

  printf("\n");
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      printf("%f", a1[i][j]);
      printf("\t");
    }
    
    //Matrix 2
    printf("|");
    printf("\t");
    printf("|");
    printf("\t");
  
    for (int k = 0; k < n; k++) {
      printf("%f", a2[i][k]);
      printf("\t");
    }
    printf("\n");
  }
  printf("----------------------------------END MATRIX OUTPUT----------------------------------");
  printf("\n");
}