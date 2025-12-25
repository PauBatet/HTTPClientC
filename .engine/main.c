#include "HTTPFramework.h"
#include "Database/Database.h"
#include "Routing/Routing.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

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
Database *db;

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
        HTTPRequest_free(&node->request);
        free(node);
    }

    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

// Handle a single request
void handle_request(HTTPRequest *request) {
    printf("Processing request: %s %s\n", request->method, request->path);

    for (int i = 0; routes[i].path != NULL; i++) {
        if (route_match(routes[i].path, request->path, request)) {
            // ---- Print query params ----
            if (request->param_count > 0) {
                printf("Query params:\n");
                for (size_t j = 0; j < request->param_count; j++) {
                    printf("  %s = %s\n",
                        request->params[j].key   ? request->params[j].key   : "(null)",
                        request->params[j].value ? request->params[j].value : "(null)");
                }
            }
            // ---- Print headers ----
            if (request->header_count > 0) {
                printf("Headers:\n");
                for (size_t j = 0; j < request->header_count; j++) {
                    printf("  %s: %s\n",
                        request->header_list[j].key   ? request->header_list[j].key   : "(null)",
                        request->header_list[j].value ? request->header_list[j].value : "(null)");
                }
            }
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
        HTTPRequest_free(&request);
    }
    return NULL;
}

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        destroy_queue(&queue);
        printf("\nClosing Database...\n");
        db_close(db);
        printf("Shutting down server...\n");
        HTTPServer_destroy(server);
        exit(0);
    }
}

void init_db() {
    //Open Database
    if (!db_open(&db)) {
        printf("Failed to open database\n");
        return;
    }
    return;
}

int run_worker() {
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    server = HTTPServer_create(SERVER_PORT);
    if (!server) {
        printf("Failed to create server\n");
        return 1;
    }

    // Initialize Database
    init_db();

    // Initialize the request queue
    init_queue(&queue);

    // Create a pool of worker threads
    pthread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&workers[i], NULL, worker_thread, NULL);
    }

    // Listening loop
    while (true) {
        HTTPRequest request = HTTPServer_listen(server);

        if (strlen(request.method) == 0) {
            printf("Invalid Request\n");
            HTTPRequest_free(&request);
            continue;
        }

        printf("New request received: %s %s\n", request.method, request.path);
        enqueue(&queue, &request);
    }

    // Cleanup (should not be reached but just in case)
    destroy_queue(&queue);
    db_close(db);
    HTTPServer_destroy(server);

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i], NULL);
    }

    return 0;
}

int main() {
    while (1) {
        pid_t pid = fork();

        if (pid == 0) {
            // CHILD → run the server normally
            exit(run_worker());
        }

        // PARENT → supervisor
        int status;
        waitpid(pid, &status, 0);

        printf("Worker died with status %d. Restarting in 1 second...\n", status);

        sleep(1);
    }
}

