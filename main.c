//GLOBAL:------------------
//Import libraries:
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

//Thread_Data Struct
typedef struct Data {
  int id;
  int start_x;
  int start_y;
  int ncells;
  int precise;
}Data;

//Function definitions--------
void print_matrices(double ** a1, double ** a2);
double ** init_matrix(double ** arr, int conf);
int getGlobalPrecision();
Data * getThreadData(int quo, int rem);
void *relax();


//ENV VARS: Maybe struct it
int n = 100;//Square matrix size
int nthreads = 2;
float precision = 0.1f;
int outsideThreshold;
int iterations;

//Init Matrices
double **a1;
double **a2;
pthread_barrier_t barrier;

//Array of thread data structs
Data *threadDataArray;


int main(void) {

  //Create Data-----------  
  a1 = init_matrix(a1, 1); //Create populated matrix
  a2 = init_matrix(a2, 0); //Create empty matrix with borders

  clock_t begin = clock();

  //Init Data Structures:
  threadDataArray = malloc(nthreads * sizeof(Data));

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

  //TIME: LINEAR
  for (int i=0; i < nthreads; i++){
	  pthread_create(&threads[i], NULL, relax, &threadDataArray[i]);
	}

  outsideThreshold = 0;

  //Loop until precision is reached
  while (1) {
    iterations++;
    pthread_barrier_wait(&barrier);
    //If precision reached then break
    if (getGlobalPrecision()) {
      outsideThreshold = 1;
      printf("Global precision reached\n");
      pthread_barrier_wait(&barrier);
      break; //exit
    }
    //print_matrices(a1, a2);
    pthread_barrier_wait(&barrier);
    }
  
    //ONLY EXITS IF GLOBAL PRECISION MET
  
  //Wait for workers to finish messing around
  //TIME: LINEAR
  for (int i=0; i < nthreads; i++){
	  pthread_join(threads[i], NULL);
	}


  for (int i = 0; i<n; i++) {
    free(a1[i]);
    free(a2[i]);
  } 
  free(a1);
  free(a2);

  free(threadDataArray);
  free(threads);

  printf("\nCompleted in %d iterations\n", iterations);

  clock_t end = clock();
  double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time spent: %f \n", time_spent);

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
  int i, j, pr, count;

  double cross;
  while (1) {
    pr = 1;
    j = sx;
    i = sy;
    //printf("\nThread: %d:Starting at [%d,%d] for %d \n", id, i, j, c);
    for (count = 0; count < c; count++) {
      cross = a1[i][j-1] + a1[i][j+1] + a1[i-1][j] + a1[i+1][j];
      a2[i][j] =  (cross/4);

      if (a2[i][j] - a1[i][j] > precision) {
        //Precision not reached for any cell
        pr = 0;
      }

      //Prepare for next iteration
      if (i == n-2) { //Onto next line: reset x & increment y
        i = 1;
        j++;
      } else { //Same line: increment x
        i++;
      }
    }

    t_data -> precise = pr;

    //Barrier
    pthread_barrier_wait(&barrier);
    pthread_barrier_wait(&barrier);

    if (outsideThreshold == 1) {
      break;
    }

    //End of iteration
    //Swap matrices
    double ** temp = a1;
    a1 = a2;
    a2 = temp;
  }
  printf("Thread %d: Terminating\n", id);
  return (NULL);
}



//-------HELPER METHODS --------

int getGlobalPrecision(){
  Data *d = threadDataArray;
	for (int i=0; i <n; i++){
    int p = d -> precise;
		if (p == 0)
			return 0;
		}
  return 1;
}

Data *getThreadData(int quo, int rem){

  int rcount = 0;
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
        xn = (int)(xn + floor(yn/(n-2))) -1;
        yn = (n-2);
      } else {
        xn = (int)(xn + floor(yn/(n-2)));
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
    thread_data.precise = 0;
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



double ** init_matrix(double **arr, int conf){

  //Allocate memory for row pointers
  arr = malloc(n * sizeof(double *));
  for (int i = 0; i < n; i++) {
    //Allocate memory for column values
    arr[i] = malloc(n * sizeof(double));
  }
  //Loop through indices of ** array
  for (int i = 0; i < n; i++){
      for (int j = 0; j < n; j++){
          //Assign value as count
          if (conf) {
            arr[i][j] = ((double)rand()/(double)RAND_MAX);
          } else {
            arr[i][j] = 0;
          }
          //Configure border numbers: 
          //Zero on bottom and right axes
          if (i == n-1){
            arr[i][j] = 0;
          }
          if (j == n-1){
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
void print_matrices(double ** a1, double ** a2) {
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