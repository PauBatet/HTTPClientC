#include "HTTPFramework.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

// Define the FIFO queue
typedef struct RequestNode {
    HTTPRequest request;
    struct RequestNode *next;
} RequestNode;

typedef struct {
    RequestNode *front;
    RequestNode *rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool stop; // Signal to stop the threads
} RequestQueue;

// Global request queue
RequestQueue queue;
HTTPServer *server;

// Initialize the request queue
void init_queue(RequestQueue *q) {
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->stop = false;
}

// Add a request to the queue
void enqueue(RequestQueue *q, HTTPRequest *request) {
    RequestNode *node = malloc(sizeof(RequestNode));
    if (!node) {
        perror("Failed to allocate memory for request node");
        return;
    }
    node->request = *request; // Copy the request
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);

    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;

    printf("Request enqueued: %s %s\n", request->method, request->path);
    pthread_cond_signal(&q->cond); // Signal a worker thread
    pthread_mutex_unlock(&q->mutex);
}

// Remove a request from the queue
bool dequeue(RequestQueue *q, HTTPRequest *request) {
    pthread_mutex_lock(&q->mutex);

    while (q->front == NULL && !q->stop) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (q->stop && q->front == NULL) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }

    RequestNode *node = q->front;
    *request = node->request; // Copy the request
    q->front = node->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(node);
    printf("Request dequeued: %s %s\n", request->method, request->path);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

// Destroy the request queue
void destroy_queue(RequestQueue *q) {
    pthread_mutex_lock(&q->mutex);
    q->stop = true;
    pthread_cond_broadcast(&q->cond); // Wake up all waiting threads
    pthread_mutex_unlock(&q->mutex);

    while (q->front) {
        RequestNode *node = q->front;
        q->front = node->next;
        free(node);
    }

    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

// Handle a single request
void handle_request(HTTPRequest *request) {
    printf("Processing request: %s %s\n", request->method, request->path);
    for (int i = 0; routes[i].path != NULL; i++) {
        if (strcmp(request->path, routes[i].path) == 0) {
            routes[i].handler(request);
            return;
        }
    }
    HTTPServer_send_response(request, "", "", 404, "<h1>404 Not Found</h1>");
}

// Worker thread function
void *worker_thread(void *arg) {
    (void)arg; // Mark parameter as unused to suppress warnings

    while (true) {
        HTTPRequest request;
        if (!dequeue(&queue, &request)) {
            break; // Stop signal received
        }
        handle_request(&request);
    }
    return NULL;
}

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down server...\n");
        destroy_queue(&queue);
        HTTPServer_destroy(server);
        exit(0);
    }
}

int main() {
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    server = HTTPServer_create(8080);
    if (!server) {
        printf("Failed to create server\n");
        return 1;
    }

    // Initialize the request queue
    init_queue(&queue);

    // Create a pool of worker threads
    const int NUM_WORKERS = 4;
    pthread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&workers[i], NULL, worker_thread, NULL);
    }

    // Listening loop
    while (true) {
        HTTPRequest request = HTTPServer_listen(server);

        if (strlen(request.method) == 0) {
            printf("Invalid Request\n");
            continue;
        }

        printf("New request received: %s %s\n", request.method, request.path);
        enqueue(&queue, &request);
    }

    // Cleanup (not reached in this example)
    destroy_queue(&queue);
    HTTPServer_destroy(server);

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i], NULL);
    }

    return 0;
}
