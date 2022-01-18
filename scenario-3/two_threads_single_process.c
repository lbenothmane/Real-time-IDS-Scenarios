/*H**********************************************************************
* FILENAME :        two_threads_single_process.c
*
* AUTHOR :    Jian Kai Lee, Mubark Jedh
* DATE :    15 Jan 2022
*H*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <Python.h>
#include </usr/include/python2.7/numpy/arrayobject.h> //Nee to change based on local path
#include "queue.h"

#define true 1
#define TIME_LIST_SIZE 100
#define NUMBER_MESSAGE_INJECTED 88492

typedef struct {
    char *can ;
}data;

struct Queue* queue;
pthread_mutex_t lock;
float list_function_time[TIME_LIST_SIZE];
int count_finish_batch;
struct timeval list_enqueue_time[TIME_LIST_SIZE];
struct timeval list_dequeue_time[TIME_LIST_SIZE];
struct timeval list_start_time[TIME_LIST_SIZE];
struct timeval list_end_time[TIME_LIST_SIZE];
struct timeval list_end_batch_time[TIME_LIST_SIZE];

/**
 * function call the python Algorithm python file
 * @param  messages               CAN messages
 * @return          1
 */
int calling_python_function(char **messages)
{
    // printf("This the being of the function \n");
    // for(int i=0;i<10;i++){
    //   printf("calling_python_function: %s\n",messages[i] );
    // }
    PyObject *pName, *pModule, *pyDict, *pFunc, *pValue, *presult, *msg ;

    Py_Initialize();
    if(!Py_IsInitialized()){
        PyErr_Print();
        printf("can't reinitialized the interperter\n");
    }

    import_array();
    PyGILState_STATE d_gstate;
    d_gstate = PyGILState_Ensure();

    PyObject *sysPath = PySys_GetObject("path");
    PyList_Append(sysPath, PyUnicode_FromString("/home/jk/Documents/Fall2021/Research_699/IDS_Reserach/C_Code/"));
    pName = PyUnicode_FromString((char*)"messages_correlation_Cosine");
    if (!pName){
      PyErr_Print();
      printf("error locating the file name ");
    }
    pModule = PyImport_Import(pName);
    if (!pModule){
      PyErr_Print();
      printf("Error in pModule");
    }
    pyDict = PyModule_GetDict(pModule);
    pFunc = PyDict_GetItemString(pyDict, (char*)"process_can_data");


    if (PyCallable_Check(pFunc))
    {
       msg = PyList_New(1000);
       Py_ssize_t size = PyList_GET_SIZE(msg);
       for ( Py_ssize_t s = 0; s<= 999; ++s){
            PyList_SetItem(msg, s, Py_BuildValue("s", messages[s]));
    }
        // printf("Let's give this a shot!\n");
       // presult = PyObject_CallFunctionObjArgs(pFunc, msg, NULL );
        PyObject_CallFunctionObjArgs(pFunc, msg, NULL );
    }
    else
    {
       PyErr_Print();
       printf("sucess but fails");
    }
    if (pValue == NULL){
        Py_DECREF(pModule);
        Py_DECREF(pName);
        Py_DECREF(msg);
        PyErr_Print();
    }

   PyGILState_Release(d_gstate);

    //Py_Finalize();
    return 1;


}

/**
 * write the total calling_python_function time to the total_time file
 */
void write_function_time(){
  FILE *fptr;
  fptr = fopen("calling_python_function_time.txt", "w");
  for(int i=0; i < TIME_LIST_SIZE; i++) {
    fprintf(fptr, "%d - The time of calling python function: %lf\n", i, list_function_time[i]);
  }
  fclose(fptr);
}

/**
 * write the rate of data loss to the file.
 */
void write_data_loss(){
  FILE *fptr;
  fptr = fopen("data_loss_rate.txt", "w");
  int data_loss = (count_finish_batch * 1000) - NUMBER_MESSAGE_INJECTED;
  fprintf(fptr, "Number of Subprocess finished: %d\n",count_finish_batch );
  fprintf(fptr, "Number of CAN message in batch: %d\n", 1000);
  fprintf(fptr, "Number of CAN message injected: %d\n", NUMBER_MESSAGE_INJECTED);
  fprintf(fptr, "Data Loss: %d\n", data_loss);
  fclose(fptr);
}

/**
 * calculate time difference funciton
 * @param  start               start time
 * @param  end                 end time
 * @return       time difference in float
 */
float time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) + 1e-6*(end->tv_usec - start->tv_usec);
}

/**
 * write time to file
 */
void write_time(){
  FILE *fptr;
  fptr = fopen("time.txt", "w");
  for(int i=0; i < TIME_LIST_SIZE; i++) {

    long start_time = list_start_time[i].tv_sec * 1000000L + list_start_time[i].tv_usec;
    long end_time = list_end_time[i].tv_sec * 1000000L + list_end_time[i].tv_usec;
    float start_end_time = time_diff(&list_start_time[i], &list_end_time[i]);
    float end_batch_end_time = time_diff(&list_end_batch_time[i], &list_end_time[i]);
    float queue_time = time_diff(&list_enqueue_time[i], &list_dequeue_time[i]);
    fprintf(fptr, "%d -Start time: %ld, End time: %ld, Time in queue: %lf, Time between End batch and End: %lf, Time between Start and End: %lf\n",
    i, start_time, end_time, queue_time, end_batch_end_time, start_end_time);
  }
  fclose(fptr);
}


/**
 * Function to free memory
 * @param arrayOfString  [pointer to array]
 * @param size           [size of array]
 */
void free_memory(char **arrayOfString, int size){
  for(int i = 0; i < size; i++){
    free(arrayOfString[i]);
  }
  free(arrayOfString);
}

void signalHandler(int signal) {
  printf("test Cought signal %d!\n", signal);
  switch (signal) {
    case SIGINT:
      printf(">>> SIGNAL INTERRUPT - calling pthread exit <<<<\n" );
      write_function_time();
      write_data_loss();
      write_time();
      pthread_exit(NULL);
  }
}

/**
 * reading can data and perform enqueue
 * @param  input               thread ID
 */
void *read_write_queue_thread(void *input) {

  long *id = (long*) input;
  // printf("read_write_queue_thread id: %ld\n", *id);
  int s , r;
  struct sockaddr_can addr;
  struct ifreq interface;
  struct canfd_frame cfd;
  int x = 0;
  data f[1000];
  int count_enqueue =0;
  struct timeval start_time;

  if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
     perror("Can't connect to Socket");
     return 1;
  }
  strcpy(interface.ifr_name, "vcan0" );
  ioctl(s, SIOCGIFINDEX, &interface);
  addr.can_family = AF_CAN;
  addr.can_ifindex = interface.ifr_ifindex;
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
              perror("Can't Bind to vcan0");
              return 1;
  }
  char **arrayOfString;
  //allocating memory for arrayOfString
  arrayOfString = malloc(1000 * sizeof(char*));
  for(int i = 0; i < 1000; i++){
    arrayOfString[i] = malloc(BUFFER_SIZE);
  }
  gettimeofday (&start_time, NULL);
  do {
      r  = read(s, &cfd, CANFD_MTU);
      if (r < 0)
      {
          perror("can raw socket read");
          return 0;
      }

      snprintf(arrayOfString[x], 100, "can_id: 0x%03X data length: %d data: %02X", cfd.can_id, cfd.len,  cfd.data);
      f[x].can = arrayOfString[x];
      x++;
      if (x == 1000){ // 0 -1000
          // printf("----------ENQUEUE-----------\n" );
          // printf("enqueuing to the queue\n" );
          // list_start_time[count_enqueue] = start_time;
          // printf("start_time: %ld\n", start_time.tv_sec);
          list_start_time[count_enqueue].tv_sec = start_time.tv_sec;
          list_start_time[count_enqueue].tv_usec = start_time.tv_usec;
          gettimeofday (&list_end_batch_time[count_enqueue], NULL);

          enqueue(queue,arrayOfString);

          gettimeofday (&list_enqueue_time[count_enqueue], NULL);
          // printf("queue size: %d\n",queue_size(queue) );
          // printf("Queue rear pointer number: %d\n", rear_pointer(queue));
          x = 0;
          count_enqueue++;

          gettimeofday (&start_time, NULL);
          // printf("----------------------------\n" );
          // free_memory(arrayOfString, 1000);
          // break;
      }
      // x++;
      }while(1);

      // closing the socket
      if (close(s) < 0) {
              perror("Unable to Close the socket ");
              return 1;
          }
}

/**
 * Dequeue from queue and pass to calling_python_function
 * @param  input               thread ID
 */
void *call_python_function_thread(void *input) {
  long *id = (long*) input;
  // printf("call_python_function_thread id: %ld\n", *id);
  struct Data current;
  struct timeval function_start_time;
  struct timeval function_end_time;

  int count =0;
  do {
    if(!isEmpty(queue)) {
      // printf("=========DEQUEUE========\n");
      // printf("Counting_debug: %d\n",count );

      //dequeue from Queue
      current = dequeue(queue);
      gettimeofday (&list_dequeue_time[count_finish_batch], NULL);
      // printf("Queue size: %d\n",queue_size(queue) );
      // printf("Queue front pointer number: %d\n", front_pointer(queue));
      gettimeofday (&function_start_time, NULL);
      int python_function_return = calling_python_function(current.listOfString);
      gettimeofday (&function_end_time, NULL);
      // printf("calling the python function() took %f seconds to execute \n", time_taken);
      list_function_time[count_finish_batch] = time_diff(&function_start_time, &function_end_time);
      // printf("count_finish_batch: %d\n", count_finish_batch);
      if (python_function_return ==-1){
          printf("Error detected\n");
       }
      else{
          printf("no problem detected\n");
      }
      gettimeofday (&list_end_time[count_finish_batch], NULL);
      count_finish_batch = count_finish_batch + 1;
      count++;
      // printf("==========================\n");
    }
  }while(1);
}

/**
 * Main
 * @return 0
 */
int main()
{

    int s , r;
    int reading_data;
    struct sockaddr_can addr;
    struct ifreq interface;
    struct canfd_frame cfd;
    int x = 0;
    data f[1000];
    clock_t t;
    pid_t pid;
    pthread_t reading_thread;
    pthread_t writing_thread;
    signal(SIGINT,signalHandler);

    printf("Program started\n");
    queue = createQueue(1000);
    count_finish_batch =0;
    pthread_create(&reading_thread, NULL, read_write_queue_thread, (void *)&reading_thread);
    pthread_create(&writing_thread, NULL, call_python_function_thread, (void *)&writing_thread);
    pthread_exit(NULL);

    return 0;
}
