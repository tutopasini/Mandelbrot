
#include <pthread.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>


// Número de threads
const int NUM_THREADS = 5;
// Buffer de trabalho
int work_buff[5000];
// Buffer de resultado
int result_buff[5000];

pthread_mutex_t mutex;
pthread_cond_t cond;

int main() {

    // Cria as threads escravas
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, calc_mandelbrot, NULL);
    }

    // Manda sinal para encerrar todas as threads
    


    // Join em todas as threads
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    return 0;
}

void *calc_mandelbrot() {

    int xi, yi, xf, yf;
    // Verifica se possui trabalho 

    
    pthread_mutex_lock(&mutex);
    
    pthread_mutex_unlock(&mutex);

    return NULL;
}