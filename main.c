#include <pthread.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>


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

int num_itens_work = 0;
int done_work = 0;
int num_itens_result = 0;
int done_result = 0;

void *mandelbrot(void *);
void *print_mandelbrot(void *);
void *insert_result();

struct point {
    int x;
    int y;
    unsigned long color;
};

struct work {
    int xi;
    int xf;
    int yi;
    int yf;
};

// Número de threads
const int NUM_THREADS = 10;
// Buffer de trabalho
struct work work_buff[10000];
// Buffer de resultado
struct point result_buff[1000000];

int main(int argc, char* argv[]) {

    xmin = -1;
    xmax = 1;
    ymin = -1;
    ymax = 1;

    xres = 400;
    yres = (xres * (ymax - ymin)) / (xmax - xmin);

    pthread_t threads[NUM_THREADS];
    pthread_t thread_printer;
    // Inicializa variáveis de condição e mutex
    pthread_mutex_init(&mutex_work, NULL);
    pthread_mutex_init(&mutex_result, NULL);
    pthread_cond_init(&cond_work, NULL);
    pthread_cond_init(&cond_result, NULL);
    // Criar as threads que irão calcular o Mandelbrot
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, mandelbrot, NULL);
    }
    // Cria a thread que irá desenhar na tela
    pthread_create(&thread_printer, NULL, print_mandelbrot, NULL);

    int xi, yi = 0;
    int xf, yf = 0;
    
    while (xf <= xres || yf <= yres) {
        xi = xf;
        yf = yi + yres / 10;

        if (xi > xres) {
            xi = 0;
            yi = yf;
        }
        xf = xi + xres / 10;

        pthread_mutex_lock(&mutex_work);

        work_buff[num_itens_work].xi = xi;
        work_buff[num_itens_work].yi = yi;
        work_buff[num_itens_work].xf = xf;
        work_buff[num_itens_work].yf = yf;
        num_itens_work++;

        pthread_cond_signal(&cond_work);
        pthread_mutex_unlock(&mutex_work);

    }
    done_work = 1;

    // Join em todas as threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    done_result = 1;

    pthread_join(thread_printer, NULL);

    return 0;
}

// Desenha na tela

void *print_mandelbrot(void *str) {

    Display * display = XOpenDisplay(NULL);

    Window janela = XCreateSimpleWindow(display, DefaultRootWindow(display),
            0, 0, xres, yres, 0, 0, 0xffffffff);

    GC gc = XCreateGC(display, janela, 0, NULL);
    XMapWindow(display, janela);

    while (!done_result || num_itens_result > 0) {

        pthread_mutex_lock(&mutex_result);
        while (num_itens_result <= 0)
            pthread_cond_wait(&cond_result, &mutex_result);
        // Desenha ponto na tela
        num_itens_result--;
        XSetForeground(display, gc, result_buff[num_itens_result].color);
        XDrawPoint(display, janela, gc,
                result_buff[num_itens_result].x,
                result_buff[num_itens_result].y);
        pthread_mutex_unlock(&mutex_result);
        XFlush(display);

    }
}

void *mandelbrot(void *str) {

    double xi, yi, xf, yf;

    while (!done_work || num_itens_work > 0) {
        pthread_mutex_lock(&mutex_work);
        while (num_itens_work <= 0)
            pthread_cond_wait(&cond_work, &mutex_work);
        num_itens_work--;
        xi = work_buff[num_itens_work].xi;
        yi = work_buff[num_itens_work].yi;
        xf = work_buff[num_itens_work].xf;
        yf = work_buff[num_itens_work].yf;
        pthread_mutex_unlock(&mutex_work);

        double dx = (xmax - xmin) / xres;
        double dy = (ymax - ymin) / yres;

        double x, y;
        int i, j;
        int k;
        for (j = yi; j < yf && j < yres; j++) {
            y = ymax - j * dy;
            for (i = xi; i < xf && i < xres; i++) {
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
                    // Coloca a cor preta no buffer de resultado
                    insert_result(i, j, 0);
                } else {
                    // Cor de fora do conjunto de Mandelbrot
//                    unsigned char color[6];
//                    color[0] = k >> 8;
//                    color[1] = k & 255;
//                    color[2] = k >> 8;
//                    color[3] = k & 255;
//                    color[4] = k >> 8;
//                    color[5] = k & 255;
                    // Coloca a cor no buffer de resultado
                    insert_result(i, j, 0xffffffff);
                }
            }
        }
    }
}

// Coloca a cor no buffer de resultado

void *insert_result(int x, int y, unsigned long color) {
    pthread_mutex_lock(&mutex_result);
    result_buff[num_itens_result].x = x;
    result_buff[num_itens_result].y = y;
    result_buff[num_itens_result].color = color;
    num_itens_result++;
    pthread_cond_signal(&cond_result);
    pthread_mutex_unlock(&mutex_result);
}