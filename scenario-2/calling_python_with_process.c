/*H**********************************************************************
* FILENAME :        calling_python_with_process.c
*
* AUTHOR :    Jian Kai Lee, Mubark Jedh
* DATE :    15 Jan 2022
*H*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <Python.h>
#include </usr/include/python2.7/numpy/arrayobject.h> //change to the local path


#define true 1
#define BUFFER_SIZE 1024
#define TIME_LIST_SIZE 100
#define NUMBER_MESSAGE_INJECTED 88492


int count_function_time;

// shared memory variable
static float *list_function_time[TIME_LIST_SIZE];
static struct timeval *list_start_time[TIME_LIST_SIZE];
static struct timeval *list_end_time[TIME_LIST_SIZE];
static struct timeval *list_end_batch_time[TIME_LIST_SIZE];

static int *count_finish_subprocess;

typedef struct {
    char *can ;
}data;

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
 * function call the python Algorithm python file
 * @param  messages               CAN messages
 * @return          1
 */
int calling_python_function(char **messages)
{
    // printf("This the being of the function \n");
    // for(int i=0;i<1000;i++){
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
    PyList_Append(sysPath, PyUnicode_FromString("/home/jk/Documents/Fall2021/Research_699/IDS_Reserach/C_Code"));
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
 * Child process to pass message data to the calling python function
 * @param messages    CAN messages
 * @param process_ID  current process ID
 */
void createChildProcess(char **messages,pid_t process_ID) {
  struct timeval function_start_time;
  struct timeval function_end_time;
  struct timeval end_time;
  // printf("CHILD PROCESS ID: %u \n",process_ID );
  // printf("count_function_time: %d\n",count_function_time );
  gettimeofday (&function_start_time, NULL);
  int python_function_return = calling_python_function(messages);
  gettimeofday (&function_end_time, NULL);

  // printf("calling the python function() took %lf seconds to execute \n", time_taken);
  *list_function_time[count_function_time] = time_diff(&function_start_time, &function_end_time);
  *count_finish_subprocess = *count_finish_subprocess + 1;
  // printf("count_finish_subprocess: %d \n", *count_finish_subprocess);
  if (python_function_return ==-1){
      printf("Error detected\n");
   }
  else{
      printf("no problem detected\n");
  }
  gettimeofday (&end_time, NULL);
  *list_end_time[count_function_time] = end_time;
  // printf("----------\n" );
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

/**
 * write the list of total calling_python_function time to the total_time file
 */
void write_function_time(){
  FILE *fptr;
  fptr = fopen("function_time.txt", "w");
  for(int i=0; i < TIME_LIST_SIZE; i++) {
    fprintf(fptr, "%d - The time of calling python function: %lf\n", i, *list_function_time[i]);
  }
  fclose(fptr);
}

/**
 * write the list of total calling_python_function time to the total_time file
 */
void write_time(){
  FILE *fptr;
  fptr = fopen("time.txt", "w");
  for(int i=0; i < TIME_LIST_SIZE; i++) {
    // double total_time_taken = ((double) *list_end_time[i] - *list_start_time[i])/CLOCKS_PER_SEC; // in seconds
    long start_time = list_start_time[i]->tv_sec * 1000000L + list_start_time[i]->tv_usec;
    long end_time = list_end_time[i]->tv_sec * 1000000L + list_end_time[i]->tv_usec;
    long end_batch_time = list_end_batch_time[i]->tv_sec * 1000000L + list_end_batch_time[i]->tv_usec;
    float start_end_time = time_diff(list_start_time[i], list_end_time[i]);
    float end_batch_end_time = time_diff(list_end_batch_time[i], list_end_time[i]);
    fprintf(fptr, "%d - Start time: %ld, End Batch time: %ld, End time: %ld, Time between End batch and End: %lf, Time between Start and End: %lf\n",
    i, start_time, end_batch_time, end_time, end_batch_end_time, start_end_time);
  }
  fclose(fptr);
}

/**
 * write the rate of data loss to the file.
 */
void write_data_loss(){
  FILE *fptr;
  fptr = fopen("data_loss_rate.txt", "w");
  int data_loss = (*count_finish_subprocess * 1000) - NUMBER_MESSAGE_INJECTED;
  fprintf(fptr, "Number of Subprocess finished: %d\n",*count_finish_subprocess );
  fprintf(fptr, "Number of CAN message in batch: %d\n", 1000);
  fprintf(fptr, "Number of CAN message injected: %d\n", NUMBER_MESSAGE_INJECTED);
  fprintf(fptr, "Data Loss: %d\n", data_loss);
  fclose(fptr);
}

void signalHandler(int signal) {
  switch (signal) {
    case SIGINT:
      printf(">>> SIGNAL INTERRUPT <<<<\n" );
      write_function_time();
      write_data_loss();
      write_time();
      exit(0);

  }
}

/**
 * Main function
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
    // clock_t t;
    pid_t pid;
    struct timeval start_time;
    struct timeval end_batch_time;
    count_function_time =0;
    signal(SIGINT,signalHandler);

    //create mapping the shared memory
    for(int i = 0; i < TIME_LIST_SIZE; i++){
      list_function_time[i] = mmap(NULL, sizeof(float), PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      list_start_time[i] = mmap(NULL, sizeof(struct timeval), PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      list_end_time[i] = mmap(NULL, sizeof(struct timeval), PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      list_end_batch_time[i] = mmap(NULL, sizeof(struct timeval), PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    }
    count_finish_subprocess = mmap(NULL, sizeof(*count_finish_subprocess), PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *count_finish_subprocess =0;
    printf("Program started\n");
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

    //allocate the memory double pointer a list of 1000 strings
    char **arrayOfString;
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
          // *list_start_time[count_function_time] = start_time;
          *list_start_time[count_function_time] = start_time;
          gettimeofday (&end_batch_time, NULL);
          *list_end_batch_time[count_function_time] = end_batch_time;
          pid = fork();

          if(pid == 0 ){

            createChildProcess(arrayOfString,getpid());
            exit(0);

          }else if(pid > 0){
            // wait(NULL);
            gettimeofday (&start_time, NULL);
            count_function_time++;

            //parent process section do nothing
          }

        x = 0;
        // time for the next one -
        // free_memory(arrayOfString, 1000); // free the array when needed
         }

        }while(1);


    // closing the socket
    if (close(s) < 0) {
            perror("Unable to Close the socket ");
            return 1;
        }


    return 0;
}
