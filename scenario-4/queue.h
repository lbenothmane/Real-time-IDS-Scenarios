// // C program for array implementation of queue
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 256

/**
 * A node of data to hold 1000 CAN messages
 */
struct Data {
  char *listOfString[1000];
};

/**
 * A structure to represent a queue
 */
struct Queue {
    int front, rear, size;
    unsigned capacity;
    struct Data *array; // array of string (char)
};

/**
 * function to create a queue of given capacity.
 * It initializes size of queue as 0
 * @param  capacity               the max number of queue can hold
 * @return          queue
 */
struct Queue* createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;

    // This is important, see the enqueue
    queue->rear = capacity - 1;

    queue->array = (struct Data*)malloc(
        queue->capacity * sizeof(struct Data));

    for(int i = 0; i < capacity; i++) {
      for(int x = 0; x < 1000; x++) {
        queue->array[i].listOfString[x] = malloc(BUFFER_SIZE);
      }
    }
    return queue;
}

// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue)
{
    return (queue->size == queue->capacity);
}

// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}

int queue_size(struct Queue* queue) {
  return queue->size;
}

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, char *item[])
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)
                  % queue->capacity;

    for(int i=0;i<1000;i++){
      strncpy(queue->array[queue->rear].listOfString[i], item[i], 100);
    }
    // write_enqueue_data(queue);
    queue->size = queue->size + 1;
    // printf("Performed enqueue\n" );
    // printf("%d enqueued to queue\n", item);
}

// Function to remove an item from queue.
// It changes front and size
struct Data dequeue(struct Queue* queue)
{
  struct Data data;
    if (isEmpty(queue))
        return data;

    struct Data item = queue->array[queue->front];
    // write_dequeue_data(queue, item);
    queue->front = (queue->front + 1)
                   % queue->capacity;
    queue->size = queue->size - 1;
    // printf("Performed Dequeue \n" );
    return item;
}

// Function to get front of queue
struct Data front(struct Queue* queue)
{
    struct Data data;
    if (isEmpty(queue))
        return data;
    return queue->array[queue->front];
}

// Function to get rear of queue
struct Data rear(struct Queue* queue)
{
    struct Data data;
    if (isEmpty(queue))
        return data;
    return queue->array[queue->rear];
}

void print_array_string(char *item[], int size) {
  for(int i=0;i < size; i++) {
    printf("%s\n",item[i] );
  }
}

void print_array_queue(struct Queue* queue,int index,int size) {
  printf("list in queue\n" );
  for(int i=0;i< size; i++) {
    printf("%d %s\n", index, queue->array[index].listOfString[i] );
  }
}

/**
 * write enqueue data for debug
 */
void write_enqueue_data(struct Queue* queue){
  FILE *fptr;
  char path[100];
  snprintf(path, sizeof(char) * 100, "/home/jk/Documents/Fall2021/Research_699/new_code/socket_process/enqueue/enqueue_%d.txt", queue->rear);
  fptr = fopen(path, "w");
  if (!fptr){
      error(EXIT_FAILURE, errno, "Failed to open file for writing");
   }
  for(int i=0;i<1000;i++){
    fprintf(fptr, "%d - %s\n", i, queue->array[queue->rear].listOfString[i]);
  }
  fclose(fptr);
}

/**
 * write dequeue data for debug
 */
void write_dequeue_data(struct Queue* queue, struct Data item){
  FILE *fptr;
  char path[100];
  snprintf(path, sizeof(char) * 100, "/home/jk/Documents/Fall2021/Research_699/new_code/socket_process/dequeue/dequeue_%d.txt", queue->front);
  fptr = fopen(path, "w");
  if (!fptr){
      error(EXIT_FAILURE, errno, "Failed to open file for writing");
   }
  for(int i=0;i<1000;i++){
    fprintf(fptr, "%d - %s\n", i, item.listOfString[i]);
  }
  fclose(fptr);
}
