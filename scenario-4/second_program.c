/*H**********************************************************************
* FILENAME :        first_program.c
*
* AUTHOR :    Jian Kai Lee, Mubark Jedh
* DATE :    15 Jan 2022
*H*/
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <Python.h>
#include </usr/include/python2.7/numpy/arrayobject.h> //Nee to change based on local path

#define PORT 8080
#define TIME_LIST_SIZE 100
#define NUMBER_MESSAGE_INJECTED 88492

// list of time
float list_function_time[TIME_LIST_SIZE];
struct timeval list_start_time[TIME_LIST_SIZE];
struct timeval list_end_batch_time[TIME_LIST_SIZE];
struct timeval list_end_time[TIME_LIST_SIZE];
float list_socket_time[TIME_LIST_SIZE];
int count_finish_function;

/**
 * function call the python Algorithm python file
 * @param  messages               [description]
 * @return          [description]
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
        printf("Let's give this a shot!\n");
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
 * write the list of total calling_python_function time to the total_time file
 */
void write_function_time(){
  FILE *fptr;
  fptr = fopen("calling_function_time_program_2.txt", "w");
  for(int i=0; i < TIME_LIST_SIZE; i++) {
    fprintf(fptr, "%d - The time of calling python function: %lf\n", i, list_function_time[i]);
  }
  fclose(fptr);
}

/**
 * write the list of time
 */
void write_time(){
  FILE *fptr;
  fptr = fopen("time_program_2.txt", "w");
  for(int i=0; i < TIME_LIST_SIZE; i++) {
    long start_time = list_start_time[i].tv_sec * 1000000L + list_start_time[i].tv_usec;
    long end_time = list_end_time[i].tv_sec * 1000000L + list_end_time[i].tv_usec;
    float start_end_time = time_diff(&list_start_time[i], &list_end_time[i]);
    float end_batch_end_time = time_diff(&list_end_batch_time[i], &list_end_time[i]);
    fprintf(fptr, "%d - Start time: %ld, Socket process time: %lf, End time: %ld, Time between End batch and End: %lf, Time between Start and End: %lf\n",
    i, start_time, list_socket_time[i], end_time, end_batch_end_time,start_end_time);
  }
  fclose(fptr);
}

/**
 * write the rate of data loss to the file.
 */
void write_data_loss(){
  FILE *fptr;
  fptr = fopen("data_loss_rate_program_2.txt", "w");
  int data_loss = (count_finish_function * 1000) - NUMBER_MESSAGE_INJECTED;
  fprintf(fptr, "Number of Subprocess finished: %d\n",count_finish_function );
  fprintf(fptr, "Number of CAN message in batch: %d\n", 1000);
  fprintf(fptr, "Number of CAN message injected: %d\n", NUMBER_MESSAGE_INJECTED);
  fprintf(fptr, "Data Loss: %d\n", data_loss);
  fclose(fptr);
}

/**
 * Ctrl C to INTERRUPT the program to write data to the files.
 * @param signal KeyboardInterrupt signal
 */
void signalHandler(int signal) {
  printf("test Cought signal %d!\n", signal);
  switch (signal) {
    case SIGINT:
      printf(">>> SIGNAL INTERRUPT - exit <<<<\n" );
      write_function_time();
      write_data_loss();
      write_time();
      exit(NULL);
  }
}

/**
 * write socket data
 */
void write_socket_data(char **messages, int number){
  FILE *fptr;
  char path[100];
  snprintf(path, sizeof(char) * 100, "/home/jk/Documents/Fall2021/Research_699/new_code/socket_process/socket_data/socket_data_%d.txt", number);
  fptr = fopen(path, "w");
  if (!fptr){
      error(EXIT_FAILURE, errno, "Failed to open file for writing");
   }
  for(int i=0;i<1000;i++){
    fprintf(fptr, "%d - %s\n", i, messages[i]);
  }
  fclose(fptr);
}

/**
 * main to listen to port 8080 and process the
 */
int main(int argc, char const *argv[])
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[50] = {0}; * @param  argc               [description]
 * @param  argv               [description]
 * @return      [description]
    char **list_can_messages;
    struct timeval start_time_taken_function;
    struct timeval end_time_taken_function;
    count_finish_function = 0;

    signal(SIGINT,signalHandler);

    //allocating memory for arrayOfString
    list_can_messages = malloc(1000 * sizeof(char*));
    for(int i = 0; i < 1000; i++){
      list_can_messages[i] = malloc(43);
    }


    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    char *ready = "READY";
    int count = 0;
    //char list_sd[8];
    if(send(sock,ready, strlen(ready), 0) < 0){
      printf("\nSend READY ERROR \n");
      return -1;
    }
    printf(">>> SENT READY\n");
    gettimeofday (&list_start_time[count_finish_function], NULL);
    while(1){
      for(int i=0; i < 1000; i++) {
        valread = read( sock , buffer, 43);
        if(valread < 0) {
          printf("\n Socket Read error \n");
          return -1;
        }
        // printf("buffer: %s\n", buffer);
        snprintf(list_can_messages[i], 100, "%s", buffer);
        // printf("count: %d, i: %d, %s\n", count, i, list_can_messages[i]);
      }
      // write_socket_data(list_can_messages, count_finish_function);
      gettimeofday (&list_end_batch_time[count_finish_function], NULL);
      list_socket_time[count_finish_function] = time_diff(&list_start_time[count_finish_function], &list_end_batch_time[count_finish_function]);
      // printf("socket_time: %ld\n", socket_time);
      // printf("----END OF 1000 LINES CAN MESSAGE----\n");
      gettimeofday (&start_time_taken_function, NULL);
      calling_python_function(list_can_messages);
      gettimeofday (&end_time_taken_function, NULL);
      list_function_time[count_finish_function] = time_diff(&start_time_taken_function, &end_time_taken_function);
      gettimeofday (&list_end_time[count_finish_function], NULL);
      count_finish_function++;
      // printf("time_taken_seconds: %lf\n", time_taken_seconds);
      // printf("count_finish_function: %d\n", count_finish_function);
      //send READY
      if(send(sock,ready, strlen(ready), 0) < 0){
        printf("\nSend READY ERROR \n");
        return -1;
      }
      gettimeofday (&list_start_time[count_finish_function], NULL);
      // printf(">>> SENT READY\n");

      count++;
    }

    if (close(sock) < 0) {
        perror("Unable to Close the socket ");
        return 1;
    }


    return 0;
}
