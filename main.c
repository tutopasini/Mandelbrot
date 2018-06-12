#include <pthread.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// Struct inserida no buffer de resultado, contendo a posição e cor

struct point {
    int x;
    int y;
    unsigned long color;
};

// Struct inserida no buffer de trabalho, contendo x e y inicial e final

struct work {
    int xi;
    int xf;
    int yi;
    int yf;
};

// Número de threads
const int NUM_THREADS = 5;
// Buffer de trabalho
struct work buff_trabalho[10000];
// Buffer de resultado
struct point buff_desenho[1000000];

void *mandelbrot(void *);
void *print_mandelbrot(void *);
void *insert_result();

pthread_mutex_t mutex_work;
pthread_mutex_t mutex_result;
pthread_cond_t cond_work;
pthread_cond_t cond_result;

// Área para cálculo do Mandelbrot
double xmin;
double xmax;
double ymin;
double ymax;

// Número de iterações de mandelbrot
const int maxiter = 65535;

// Tamanho da janela
const int xres = 400;
// Calcula automático com base em xres e xmin/xmax/ymin/ymax
int yres;

// Quantidade de itens no buffer de trabalho
int num_itens_work = 0;
// Indica se parou de adicionar no buffer de trabalho
int done_work = 0;
// Quantidade de itens no buffer de resultado
int num_itens_result = 0;
// Indica se parou de adicionar no buffer de resultado
int done_result = 0;

int main(int argc, char* argv[]) {

    // Define área que será calculada
    xmin = -0.5;
    xmax = 0.5;
    ymin = -0.5;
    ymax = 0.5;

    // Calcula a resolução Y
    yres = (xres * (ymax - ymin)) / (xmax - xmin);

    // Inicializa threads
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

    int xi = 0, yi = 0;
    int xf = 0, yf = 0;

    // Laço de inserção no buffer de trabalho
    // Buffer que divide a tela em quadrados (pontos iniciais e finais)
    while (xf <= xres || yf <= yres) {
        xi = xf;

        if (xi > xres) {
            xi = 0;
            yi = yf;
        }
        xf = xi + xres / 10;
        yf = yi + yres / 10;
        
        pthread_mutex_lock(&mutex_work);

        // Insere no buffer de trabalho
        buff_trabalho[num_itens_work].xi = xi;
        buff_trabalho[num_itens_work].yi = yi;
        buff_trabalho[num_itens_work].xf = xf;
        buff_trabalho[num_itens_work].yf = yf;
        num_itens_work++;

        // Envia o sinal
        pthread_cond_signal(&cond_work);
        pthread_mutex_unlock(&mutex_work);

    }
    done_work = 1;

    // Join em todas as threads
    // Aguarda as threads finalizarem
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    // Indica que parou de adicionar trabalho
    done_result = 1;
    pthread_mutex_destroy(&mutex_work);
    pthread_cond_destroy(&cond_work);
    
    // Libera a thread printer (pode estar aguardando a variável de condição)
    pthread_cond_signal(&cond_result);
    // Join na thread printer
    pthread_join(thread_printer, NULL);
    pthread_mutex_destroy(&mutex_result);
    pthread_cond_destroy(&cond_result);
    sleep(5);

    return 0;
}

// Desenha na tela

void *print_mandelbrot(void *str) {

    // Cria display
    Display * display = XOpenDisplay(NULL);

    // Cria janela
    Window janela = XCreateSimpleWindow(display, DefaultRootWindow(display),
            0, 0, xres, yres, 0, 0, 0xffffffff);

    // Cria GC
    GC gc = XCreateGC(display, janela, 0, NULL);

    // Mapeia display/janela
    XMapWindow(display, janela);

    // Laço para consumir o buffer de resultado e desenhar na tela
    while (!done_result || num_itens_result > 0) {

        pthread_mutex_lock(&mutex_result);
        while (num_itens_result <= 0) {
            pthread_cond_wait(&cond_result, &mutex_result);
            // Se finalizou e não tem mais o que desenhar
            if (done_result && num_itens_result <= 0)
                return 0;
        }
        num_itens_result--;
        // Define cor do ponto
        XSetForeground(display, gc, buff_desenho[num_itens_result].color);
        // Desenha ponto na tela
        XDrawPoint(display, janela, gc,
                buff_desenho[num_itens_result].x,
                buff_desenho[num_itens_result].y);
        pthread_mutex_unlock(&mutex_result);
        XFlush(display);
    }
    return 0;
}

// Cálculo de Mandelbrot

void *mandelbrot(void *str) {

    double xi, yi, xf, yf;
    // Enquanto não parou de adicionar trabalho e houver trabalho
    while (!done_work || num_itens_work > 0) {

        // Bloqueia acesso ao buffer de trabalho
        pthread_mutex_lock(&mutex_work);
        while (num_itens_work <= 0) {
            // Aguarda até receber um sinal
            pthread_cond_wait(&cond_work, &mutex_work);
            // Se parou de adicionar trabalho e não tem mais trabalho
            if (done_work && num_itens_work <= 0)
                return 0;
        }
        // Busca do buffer de trabalho
        num_itens_work--;
        xi = buff_trabalho[num_itens_work].xi;
        yi = buff_trabalho[num_itens_work].yi;
        xf = buff_trabalho[num_itens_work].xf;
        yf = buff_trabalho[num_itens_work].yf;
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
                    // Coloca a cor branca no buffer de resultado
                    insert_result(i, j, 0xffffffff);
                }
            }
        }
    }
    return 0;
}

// Coloca a cor no buffer de resultado

void *insert_result(int x, int y, unsigned long color) {
    pthread_mutex_lock(&mutex_result);
    buff_desenho[num_itens_result].x = x;
    buff_desenho[num_itens_result].y = y;
    buff_desenho[num_itens_result].color = color;
    num_itens_result++;
    pthread_cond_signal(&cond_result);
    pthread_mutex_unlock(&mutex_result);
}