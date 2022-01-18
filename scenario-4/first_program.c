/*H**********************************************************************
* FILENAME :        first_program.c
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
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <Python.h>
#include </usr/include/python2.7/numpy/arrayobject.h>
#include "queue.h"


#define true 1
#define PORT 8080
#define TIME_LIST_SIZE 100

struct Queue* queue;
int count_finish_batch;
struct timeval list_enqueue_time[TIME_LIST_SIZE]; //the time right after performed enqueue
struct timeval list_dequeue_time[TIME_LIST_SIZE]; //the time right after performed dequeue
struct timeval list_start_time[TIME_LIST_SIZE]; //start collect data time
struct timeval list_end_time[TIME_LIST_SIZE]; //the time after send to process 2
struct timeval list_end_batch_time[TIME_LIST_SIZE]; //when there is 1000 line and ready to enqueue
float list_socket_time[TIME_LIST_SIZE]; //how much time socket take to send

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
 * write queue time to file
 */
void write_time(){
  FILE *fptr;
  fptr = fopen("time_program_1.txt", "w");
  for(int i=0; i < TIME_LIST_SIZE; i++) {

    long start_time = list_start_time[i].tv_sec * 1000000L + list_start_time[i].tv_usec;
    long end_time = list_end_time[i].tv_sec * 1000000L + list_end_time[i].tv_usec;
    float start_end_time = time_diff(&list_start_time[i], &list_end_time[i]);
    float end_batch_end_time = time_diff(&list_end_batch_time[i], &list_end_time[i]);
    float queue_time = time_diff(&list_enqueue_time[i], &list_dequeue_time[i]);
    fprintf(fptr, "%d -Start time: %ld End time: %ld, Time taken to send socket: %lf, Time in queue: %lf, Time between End batch and End: %lf, Time between Start and End: %lf\n",
    i, start_time, end_time, list_socket_time[i], queue_time, end_batch_end_time, start_end_time);
  }
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
      printf(">>> SIGNAL INTERRUPT - calling pthread exit <<<<\n" );
      write_time();
      pthread_exit(NULL);
  }
}

/**
 * reading can data and perform enqueue
 * @param  input               [description]
 * @return       [description]
 */
void *reading_can_thread(void *input) {

  long *id = (long*) input;
  printf("reading_can_thread id: %ld\n", *id);
  int s , r;
  struct sockaddr_can addr;
  struct ifreq interface;
  struct canfd_frame cfd;
  int x = 0;
  struct timeval start_time;
  int count_enqueue =0;

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
      //printf("cfd.can_id: %03X\n", cfd.can_id);

      snprintf(arrayOfString[x], 100, "can_id: 0x%03X data length: %d data: %02X", cfd.can_id, cfd.len,  cfd.data);

      x++;
      if (x == 1000){ // 1 -1000
          // printf("-------ENQUEUE-----------\n" );

          list_start_time[count_enqueue] = start_time;
          gettimeofday (&list_end_batch_time[count_enqueue], NULL);
          enqueue(queue,arrayOfString);
          gettimeofday (&list_enqueue_time[count_enqueue], NULL);
          // printf("enqueue size: %d\n",queue_size(queue) );
          x = 0;
          count_enqueue++;
          gettimeofday (&start_time, NULL);
          // printf("---------------------%d\n",x );
          // free_memory(arrayOfString, 1000);
          // break;
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
    // data f[1000];
    struct timeval start_send_time_taken;
    struct timeval end_send_time_taken;
    pid_t pid;
    pthread_t reading_thread;
    count_finish_batch = 0;
    signal(SIGINT,signalHandler);

    printf("Program started\n");
    queue = createQueue(1000);
    pthread_create(&reading_thread, NULL, reading_can_thread, (void *)&reading_thread);

    printf("-----SOCKET PARTS-----\n" );
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Hello from server\n";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("----ESTABLISH SOCKET CONNECTION----\n" );
    int count =0;

    //test send socket message
    char* str1 = "Hello from server";
    char text[2];


    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen))<0)
    {
        perror("SOCKET ACCEPT ERROR");
        exit(EXIT_FAILURE);
    }
    printf("----SOCKET ACCEPT----\n" );

    while((valread = read(new_socket, buffer, 5)) > 0 ){
      // printf("buffer messge: %s, isEmpty: %d\n", buffer, !isEmpty(queue));
      // printf("current queue size: %d\n",queue_size(queue) );

      if(!strcmp(buffer,"READY")) {
        while(1) {
          if(!isEmpty(queue)) {
            // printf(">>>>>>Dequeue>>>>>>>>>\n" );
            // printf("queue size before dequeue: %d\n",queue_size(queue) );
            struct Data current = dequeue(queue);
            gettimeofday (&list_dequeue_time[count], NULL);
            // printf(" current.listOfString[1]: %ld\n",strlen(current.listOfString[1]) );
            gettimeofday (&start_send_time_taken, NULL);
            for(int i = 0; i < 1000; i++) {
              // printf("count: %d, i: %d size: %ld --- %s\n", count, i, strlen(current.listOfString[i]),current.listOfString[i]);
              send(new_socket , current.listOfString[i] , strlen(current.listOfString[i]) , 0 );
            }
            gettimeofday (&end_send_time_taken, NULL);
            list_socket_time[count] = time_diff(&start_send_time_taken, &end_send_time_taken);
            // printf("send_time_taken: %lf\n", time_taken_seconds);
            gettimeofday (&list_end_time[count], NULL);
            // printf("current size : %ld\n", sizeof(current));
            count++;
            // printf(">>>>>>>>>>>>>>>>>>>>>\n" );
            break;
          }
        }
        //break;
      }
    }
    printf(">>> END WHILE\n" );
    // closing the socket
    if (close(s) < 0) {
            perror("Unable to Close the socket ");
            return 1;
    }
    pthread_exit(NULL);
    printf("----- PROGRAM END--------\n");



    return 0;
}
