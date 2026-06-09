/* main.cpp
   Programa principal para detección de círculos en imágenes BMP
   usando un Algoritmo Genético Simple
   Integrante 1: Edgar
   Modificado para Integrante 2 (Automatización y Sintonización)
   Modificado por integrante 3 (ejecucion y estadisticas)
   Basado en gcIMG del Dr. Carlos Hugo García Capulín
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "sga.hpp"

typedef unsigned char      BYTE;
typedef unsigned short int word;
typedef unsigned long  int dword;

typedef struct{
    BYTE   id[2];
    word   offset;
    word   ancho;
    word   alto;
    BYTE   bpp;
    int    size;
    BYTE  *head;
    float *imx;
} gcIMG;

gcIMG* gcNewImg(int ancho, int alto)
{
    gcIMG *img = (gcIMG*)calloc(1, sizeof(gcIMG));
    img->id[0] = 'B'; img->id[1] = 'M';
    img->ancho = ancho;
    img->alto  = alto;
    img->bpp   = 8;
    img->size  = ancho * alto;
    img->head  = (BYTE*)calloc(54, 1);
    img->imx   = (float*)calloc(ancho * alto, sizeof(float));
    int i;
    for (i = 0; i < ancho * alto; i++)
        img->imx[i] = 255.0f;
    return img;
}

gcIMG* gcGetImgBmp(char *ruta)
{
    gcIMG *img;
    FILE *file;
    int i, j;
    BYTE pal[1024];
    int tam_fila, padding;

    file = fopen(ruta, "rb");
    if (!file) { fprintf(stderr, "Error al abrir %s\n", ruta); return NULL; }

    img = (gcIMG*)calloc(1, sizeof(gcIMG));
    if (!img) { fclose(file); return NULL; }

    fread(img->id, 2, 1, file);
    fseek(file, 10, SEEK_SET);
    fread(&img->offset, 2, 1, file);
    fseek(file, 18, SEEK_SET);
    fread(&img->ancho, 2, 1, file);
    fseek(file, 22, SEEK_SET);
    fread(&img->alto, 2, 1, file);
    fseek(file, 28, SEEK_SET);
    fread(&img->bpp, 1, 1, file);
    fseek(file, 34, SEEK_SET);
    fread(&img->size, 4, 1, file);

    if (img->id[0] != 'B' || img->id[1] != 'M') {
        fprintf(stderr, "Error: No es BMP\n");
        fclose(file); free(img); return NULL;
    }

    img->head = (BYTE*)malloc(54);
    fseek(file, 0, SEEK_SET);
    fread(img->head, 1, 54, file);

    fseek(file, 54, SEEK_SET);
    fread(pal, 1, 1024, file);

    img->imx = (float*)malloc(img->ancho * img->alto * sizeof(float));
    if (!img->imx) { fclose(file); free(img->head); free(img); return NULL; }

    padding = (4 - (img->ancho % 4)) % 4;
    tam_fila = img->ancho + padding;

    BYTE *fila = (BYTE*)malloc(tam_fila);
    for (i = img->alto - 1; i >= 0; i--) {
        fread(fila, 1, tam_fila, file);
        for (j = 0; j < img->ancho; j++)
            img->imx[i * img->ancho + j] = (float)fila[j];
    }
    free(fila);
    fclose(file);
    return img;
}

void gcPutImgBmp24(char *ruta, gcIMG *img, int *circ_x, int *circ_y, int n_circ)
{
    FILE *file;
    int i, j, padding, tam_fila;

    file = fopen(ruta, "wb");
    if (!file) return;

    BYTE head[54];
    memcpy(head, img->head, 54);
    head[28] = 24;
    *(int*)(head+10) = 54;
    fwrite(head, 1, 54, file);

    padding = (4 - ((img->ancho * 3) % 4)) % 4;
    tam_fila = img->ancho * 3 + padding;

    BYTE *fila = (BYTE*)malloc(tam_fila);
    memset(fila, 0, tam_fila);

    for (i = img->alto - 1; i >= 0; i--) {
        for (j = 0; j < img->ancho; j++) {
            BYTE val = (BYTE)img->imx[i * img->ancho + j];
            int es_circulo = 0;
            for (int c = 0; c < n_circ; c++)
                if (circ_x[c] == j && circ_y[c] == i) { es_circulo = 1; break; }
            if (es_circulo) {
                fila[j*3+0] = 0;     // B
                fila[j*3+1] = 0;     // G
                fila[j*3+2] = 255;   // R
            } else {
                fila[j*3+0] = val;
                fila[j*3+1] = val;
                fila[j*3+2] = val;
            }
        }
        fwrite(fila, 1, tam_fila, file);
    }
    free(fila);
    fclose(file);
}

void gcFreeImg(gcIMG *img)
{
    if (img) {
        if (img->head) free(img->head);
        if (img->imx)  free(img->imx);
        free(img);
    }
}

static int bits_necesarios(int n)
{
    int bits = 0, t = n - 1;
    while (t > 0) { bits++; t >>= 1; }
    return bits > 0 ? bits : 1;
}

int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL));

    char nom_img[256] = "imgs/C01.bmp";
    if (argc > 1) strcpy(nom_img, argv[1]);

    // Redireccionar mensajes informativos a stderr
    fprintf(stderr, "Cargando: %s\n", nom_img);

    gcIMG *img = gcGetImgBmp(nom_img);
    if (!img) { fprintf(stderr, "ERROR: No se pudo abrir %s\n", nom_img); return 1; }

    int ancho = img->ancho, alto = img->alto;
    fprintf(stderr, "Dimensiones: %d x %d\n", ancho, alto);

    float *original_img = (float*)malloc(ancho * alto * sizeof(float));
    if (!original_img) { fprintf(stderr, "Error de memoria\n"); return 1; }
    memcpy(original_img, img->imx, ancho * alto * sizeof(float));

    float sobel_th = 30.0f;
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            float gx = (x == 0 || x == ancho-1) ? 0.0f :
                       original_img[y * ancho + x + 1] - original_img[y * ancho + x - 1];
            float gy = (y == 0 || y == alto-1) ? 0.0f :
                       original_img[(y-1) * ancho + x] - original_img[(y+1) * ancho + x];
            img->imx[y * ancho + x] = (sqrtf(gx*gx + gy*gy) > sobel_th) ? 0.0f : 255.0f;
        }
    }
    fprintf(stderr, "Bordes Sobel umbral=%.0f\n", sobel_th);

    int total = 0, i, j, k;
    for (i = 0; i < alto; i++)
        for (j = 0; j < ancho; j++)
            if (img->imx[i * ancho + j] == 0.0f)
                total++;

    if (total < 3) {
        fprintf(stderr, "ERROR: Se requieren al menos 3 pixeles negros\n");
        return 1;
    }

    int *px = (int*)malloc(total * sizeof(int));
    int *py = (int*)malloc(total * sizeof(int));
    if (!px || !py) { fprintf(stderr, "Error de memoria\n"); return 1; }

    k = 0;
    for (i = 0; i < alto; i++)
        for (j = 0; j < ancho; j++)
            if (img->imx[i * ancho + j] == 0.0f) {
                px[k] = j;
                py[k] = i;
                k++;
            }

    fprintf(stderr, "Pixeles negros: %d\n", total);

    int bits = bits_necesarios(total);
    fprintf(stderr, "Bits por gen: %d\n", bits);

    const unsigned int NUM_GENES = 3;
    unsigned int nbits[3] = { (unsigned int)bits, (unsigned int)bits, (unsigned int)bits };
    float ls[3] = { (float)(total-1), (float)(total-1), (float)(total-1) };
    float li[3] = { 0.0f, 0.0f, 0.0f };

    // Valores por defecto
    unsigned int POB = 100;
    unsigned int GENS = 500;
    float PC = 0.90f;
    float PM = 0.05f;
    int metodo_seleccion = 2; // 1: Ruleta, 2: Torneo
    int tipo_cruza = 1;

    // Sobrescritura por linea de comandos
    if (argc >= 3) POB = atoi(argv[2]);
    if (argc >= 4) GENS = atoi(argv[3]);
    if (argc >= 5) PC = atof(argv[4]);
    if (argc >= 6) PM = atof(argv[5]);
    if (argc >= 7) metodo_seleccion = atoi(argv[6]);
    if (argc >= 8) tipo_cruza = atoi(argv[7]);

    fprintf(stderr, "Poblacion: %u, Generaciones: %u, Cruza: %.2f, Mut: %.2f, Sel: %d, TipoCruza: %d\n", POB, GENS, PC, PM, metodo_seleccion, tipo_cruza);

    int iteraciones = (argc > 8) ? atoi(argv[8]) : 100;
    float sum_cx = 0, sum_cy = 0, sum_r = 0, sum_fit = 0;
    float sq_cx = 0, sq_cy = 0, sq_r = 0, sq_fit = 0;

    float best_fit = -1.0f, best_cx = 0, best_cy = 0, best_r = 0;

    for(int e = 0; e < iteraciones; e++) {
        GA ga(POB, NUM_GENES, nbits, ls, li, PC, PM);
        ga.setTipoFuncion(5);
        ga.setOptDir(MAX);
        ga.setMetodoSeleccion(metodo_seleccion);
        ga.setTournamentSize(2);
        ga.setTipoCruza(tipo_cruza);
        ga.setPixelNegros(px, py, total, img->imx, ancho, alto);

        ga.Evolucionar(GENS);

        float cx, cy, r;
        ga.GetBestCirculo(&cx, &cy, &r);
        float fit = ga.GetBestObj();

        // Salida individual para el archivo CSV
        printf("%.2f,%.2f,%.2f,%.0f\n", cx, cy, r, fit);

        // Acumulación para estadísticas
        sum_cx += cx; sq_cx += cx * cx;
        sum_cy += cy; sq_cy += cy * cy;
        sum_r += r;   sq_r += r * r;
        sum_fit += fit; sq_fit += fit * fit;

        // Guardar el mejor círculo (mayor fitness)
        if (fit > best_fit) {
            best_fit = fit;
            best_cx = cx;
            best_cy = cy;
            best_r = r;
        }
    }

    // promedios y desviaciones estandar poblacionales
    float avg_cx = sum_cx / iteraciones;
    float sd_cx = sqrt((sq_cx / iteraciones) - (avg_cx * avg_cx));
    
    float avg_cy = sum_cy / iteraciones;
    float sd_cy = sqrt((sq_cy / iteraciones) - (avg_cy * avg_cy));
    
    float avg_r = sum_r / iteraciones;
    float sd_r = sqrt((sq_r / iteraciones) - (avg_r * avg_r));
    
    float avg_fit = sum_fit / iteraciones;
    float sd_fit = sqrt((sq_fit / iteraciones) - (avg_fit * avg_fit));

    //línea identificadora de estadisticas para python
    printf("STATS,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", 
            avg_cx, sd_cx, avg_cy, sd_cy, avg_r, sd_r, avg_fit, sd_fit);

    // Restaurar imagen original y dibujar el mejor círculo en rojo
    memcpy(img->imx, original_img, ancho * alto * sizeof(float));

    int circ_x[1080], circ_y[1080], n_circ = 0;
    if (best_fit > 80) {
        for (int a = 0; a < 360; a++) {
            float rad = (float)a * 3.14159265f / 180.0f;
            for (int d = -1; d <= 1; d++) {
                float rd = (float)(int)(best_r + d);
                if (rd < 1.0f) continue;
                int x = (int)(best_cx + rd * cosf(rad));
                int y = (int)(best_cy + rd * sinf(rad));
                if (x >= 0 && x < ancho && y >= 0 && y < alto) {
                    if (n_circ < 1080) { circ_x[n_circ] = x; circ_y[n_circ] = y; n_circ++; }
                }
            }
        }
    }

    system("mkdir -p outputs");
    char res_nom[512];
    char *solo_nombre = strrchr(nom_img, '/');
    if (solo_nombre) solo_nombre++;
    else             solo_nombre = nom_img;
    snprintf(res_nom, 512, "outputs/%s", solo_nombre);
    char *p = strrchr(res_nom, '.');
    if (p) strcpy(p, "_resultado.bmp");
    else   strcat(res_nom, "_resultado.bmp");
    gcPutImgBmp24(res_nom, img, circ_x, circ_y, n_circ);
    fprintf(stderr, "Guardado: %s\n", res_nom);

    free(original_img);
    gcFreeImg(img);
    free(px); free(py);
    return 0;
}