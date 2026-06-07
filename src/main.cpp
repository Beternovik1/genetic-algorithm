/* main.cpp
   Programa principal para detección de círculos en imágenes BMP
   usando un Algoritmo Genético Simple
   Integrante 1: Edgar
   Basado en gcIMG del Dr. Carlos Hugo García Capulín
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "sga.hpp"

/* **************************************************************************
   PROCESAMIENTO DIGITAL DE IMAGENES
   Biblioteca Basica de Funciones
   Autor: Dr. Carlos Hugo Garcia Capulin
   Ver 1.2
   Prohibido su uso, distribucion y copia sin autorizacion por parte de
   DIVISION DE INGENIERIAS CAMPUS IRAPUATO-SALAMANCA
   UNIVERSIDAD DE GUANAJUATO
************************************************************************** */

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
    if (!file) { printf("Error al abrir %s\n", ruta); return NULL; }

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
        printf("Error: No es BMP\n");
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

void gcPutImgBmp(char *ruta, gcIMG *img)
{
    FILE *file;
    int i, j, padding, tam_fila;
    BYTE pal[1024];

    file = fopen(ruta, "wb");
    if (!file) return;

    fwrite(img->head, 1, 54, file);
    for (i = 0; i < 256; i++) {
        pal[i*4+0] = pal[i*4+1] = pal[i*4+2] = (BYTE)i;
        pal[i*4+3] = 0;
    }
    fwrite(pal, 1, 1024, file);

    padding = (4 - (img->ancho % 4)) % 4;
    tam_fila = img->ancho + padding;

    BYTE *fila = (BYTE*)malloc(tam_fila);
    for (i = img->alto - 1; i >= 0; i--) {
        for (j = 0; j < img->ancho; j++)
            fila[j] = (BYTE)img->imx[i * img->ancho + j];
        for (j = img->ancho; j < tam_fila; j++)
            fila[j] = 0;
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

/* Calcula cuantos bits se necesitan para indexar N elementos */
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

    printf("Cargando: %s\n", nom_img);

    gcIMG *img = gcGetImgBmp(nom_img);
    if (!img) { printf("ERROR: No se pudo abrir %s\n", nom_img); return 1; }

    int ancho = img->ancho, alto = img->alto;
    printf("Dimensiones: %d x %d\n", ancho, alto);

    /* Contar pixeles negros (imx[i] == 0.0f) */
    int total = 0, i, j, k;
    for (i = 0; i < alto; i++)
        for (j = 0; j < ancho; j++)
            if (img->imx[i * ancho + j] == 0.0f)
                total++;

    if (total < 3) {
        printf("ERROR: Se requieren al menos 3 pixeles negros\n");
        return 1;
    }

    int *px = (int*)malloc(total * sizeof(int));
    int *py = (int*)malloc(total * sizeof(int));
    if (!px || !py) { printf("Error de memoria\n"); return 1; }

    k = 0;
    for (i = 0; i < alto; i++)
        for (j = 0; j < ancho; j++)
            if (img->imx[i * ancho + j] == 0.0f) {
                px[k] = j;
                py[k] = i;
                k++;
            }

    printf("Pixeles negros: %d\n", total);
    int n = total < 10 ? total : 10;
    for (i = 0; i < n; i++)
        printf("  pixel[%d]: (%d, %d)\n", i, px[i], py[i]);

    /* Configurar AG */
    int bits = bits_necesarios(total);
    printf("Bits por gen: %d\n", bits);

    const unsigned int NUM_GENES = 3;
    unsigned int nbits[3] = { (unsigned int)bits, (unsigned int)bits, (unsigned int)bits };
    float ls[3] = { (float)(total-1), (float)(total-1), (float)(total-1) };
    float li[3] = { 0.0f, 0.0f, 0.0f };

    const unsigned int POB = 100;
    const unsigned int GENS = 500;
    const float PC = 0.90f, PM = 0.05f;

    printf("Poblacion: %u, Generaciones: %u, Cruza: %.2f, Mut: %.2f\n", POB, GENS, PC, PM);

    GA ga(POB, NUM_GENES, nbits, ls, li, PC, PM);
    ga.setTipoFuncion(5);
    ga.setOptDir(MAX);
    ga.setMetodoSeleccion(2);
    ga.setTournamentSize(2);
    ga.setTipoCruza(1);
    ga.setPixelNegros(px, py, total, img->imx, ancho, alto);

    ga.Evolucionar(GENS);

    float cx, cy, r;
    ga.GetBestCirculo(&cx, &cy, &r);
    float fit = ga.GetBestObj();

    printf("\n=== RESULTADO ===\n");
    printf("Centro: (%.2f, %.2f)\n", cx, cy);
    printf("Radio:  %.2f\n", r);
    printf("Fitness: %.0f / 360 (%.1f%%)\n", fit, (fit / 360.0f) * 100.0f);

    /* Dibujar circulo detectado en la imagen */
    for (int a = 0; a < 360; a++) {
        float rad = (float)a * 3.14159265f / 180.0f;
        int x = (int)(cx + r * cosf(rad));
        int y = (int)(cy + r * sinf(rad));
        if (x >= 0 && x < ancho && y >= 0 && y < alto)
            img->imx[y * ancho + x] = 128.0f;
    }

    /* Guardar resultado */
    system("mkdir -p outputs");
    char res_nom[512];
    char *solo_nombre = strrchr(nom_img, '/');
    if (solo_nombre) solo_nombre++;
    else             solo_nombre = nom_img;
    snprintf(res_nom, 512, "outputs/%s", solo_nombre);
    char *p = strrchr(res_nom, '.');
    if (p) strcpy(p, "_resultado.bmp");
    else   strcat(res_nom, "_resultado.bmp");
    gcPutImgBmp(res_nom, img);
    printf("Guardado: %s\n", res_nom);

    gcFreeImg(img);
    free(px); free(py);
    return 0;
}
