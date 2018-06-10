
#include <pthread.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>


// Número de threads
const int NUM_THREADS = 1;
// Buffer de trabalho
int work_buff[5000];
// Buffer de resultado
int *result_buff[5000][5000];

pthread_mutex_t mutex_work;
pthread_mutex_t mutex_result;
pthread_cond_t cond_work;
pthread_cond_t cond_result;

// Bordas da janela
double xmin;
double xmax;
double ymin;
double ymax;

// Número de iterações de mandelbrot
const int maxiter = 65535;

// Tamanho da janela
int xres;
int yres;

int num_itens_result = 0;

void *mandelbrot();
void *insert_result();

int main(int argc, char* argv[]) {

    xmin = atof(argv[1]);
    xmax = atof(argv[2]);
    ymin = atof(argv[3]);
    ymax = atof(argv[4]);

    xres = atoi(argv[5]);
    yres = (xres * (ymax - ymin)) / (xmax - xmin);

    // Cria as threads escravas
    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex_work, NULL);
    pthread_mutex_init(&mutex_result, NULL);
    pthread_cond_init(&cond_work, NULL);
    pthread_cond_init(&cond_result, NULL);
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, mandelbrot, NULL);
    }

    // pthread_mutex_lock(&mutex_work);
    // TODO: Aqui deve preencher o buffer de trabalho
    // num_itens++;
    // pthread_cond_signal(&cond_work);
    // pthread_mutex_unlock(&mutex_work);


    // Mandar sinal para encerrar todas as threads



    // Join em todas as threads
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    return 0;
}

void *mandelbrot() {

    double dx = (xmax - xmin) / xres;
    double dy = (ymax - ymin) / yres;

    double x, y;
    double u, v;
    int i, j;
    int k;
    for (j = 0; j < yres; j++) {
        y = ymax - j * dy;
        for (i = 0; i < xres; i++) {
            double u = 0.0;
            double v = 0.0;
            double u2 = u * u;
            double v2 = v*v;
            x = xmin + i * dx;

            for (k = 1; k < maxiter && (u2 + v2 < 4.0); k++) {
                v = 2 * u * v + y;
                u = u2 - v2 + x;
                u2 = u * u;
                v2 = v * v;
            };
            // Se está dentro do conjunto de Mandelbrot
            if (k >= maxiter) {
                int color[] = {0, 0, 0, 0, 0, 0};
                // Coloca a cor preta no buffer de resultado
                insert_result(i, j, color);
            } else {
                // Cor de fora do conjunto de Mandelbrot
                unsigned char color[6];
                color[0] = k >> 8;
                color[1] = k & 255;
                color[2] = k >> 8;
                color[3] = k & 255;
                color[4] = k >> 8;
                color[5] = k & 255;
                // Coloca a cor no buffer de resultado
                insert_result(i, j, color);
            }
        }
    }

    return NULL;

}

// Coloca a cor no buffer de resultado
void *insert_result(int i, int j, int color[6]) {
    pthread_mutex_lock(&mutex_result);
    result_buff[i][j] = color;
    num_itens_result++;
    pthread_cond_signal(&cond_result);
    pthread_mutex_unlock(&mutex_result);
}