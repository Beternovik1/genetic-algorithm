/* sga.hpp
   Archivo de Cabecera del Algoritmo Genético Simple
   Modificado para detección de círculos en imágenes BMP
 */
#ifndef SGA_HPP
#define SGA_HPP

#include <iostream>
#include <cmath>
#include <fstream>
using namespace std;

typedef unsigned char BYTE;
typedef enum { MAX, MIN } OPT_TYPE;

typedef struct {
    BYTE         *Chrom; 
    unsigned int *Vent;    
    float        *Vre;    
    float         VObj;    
    float         VFit;
    unsigned int Padre1;
    unsigned int Padre2;    
} INDIVIDUO;

/* Estructura para almacenar coordenadas de píxeles negros */
typedef struct {
    int *x;
    int *y;
    int count;
} PIXELES_NEGROS;

class GA {
private:
    INDIVIDUO          *POB;        
    INDIVIDUO          *TmpPOB;     
    unsigned int       *Seleccion;
    unsigned int       *SelPool;    
    BYTE               *EliteChrom; 
    unsigned int        ChromeSize;
    unsigned int        NumGens;
    const unsigned int *BitxGen;
    unsigned int        PobSize;
    const float        *LSup;
    const float        *LInf;
    int                 tipo_funcion;
    const float* X_data;
    const float* Y_data;
    int                 num_puntos;
    unsigned int        Id_BestObj;
    unsigned int        Id_WorstObj;
    float               SumObj;
    float               PromObj;
    float               SumFit;
    float               PromFit;
    float               ProbCruza;
    float               ProbMutacion;
    unsigned int        tournament_size;
    int                 tipo_cruza;
    
    /* Controles añadidos */
    OPT_TYPE            opt_dir;
    int                 metodo_seleccion; /* 1 = Ruleta, 2 = Torneo */
    bool                elitismo;

    /* Datos para detección de círculos (tipo_funcion = 5) */
    int                *pixel_x;
    int                *pixel_y;
    int                 cant_pixeles_negros;
    float              *imagen_plana;
    int                 ancho_img;
    int                 alto_img;

    void AllocIndividuo(INDIVIDUO &ind);
    int funcion_objetivo_circulo(int cx, int cy, int r);

public:
    GA(unsigned int T_Pob,
       unsigned int N_Gens,
       const unsigned int *NBsxGen,
       const float        *L_S,
       const float        *L_I,
       float               ProbC = 0.95f,
       float               ProbM = 0.005f);
    ~GA();

    void DecodeEnt(void);
    void DecodeReal(void);
    void EvaluarPob(void);
    float FuncionObjetivo(unsigned int Id);
    void Obj_to_Fit(OPT_TYPE Tipo);
    void Roulette(void); 
    void Cruza(void);      
    void Mutacion(void);  
    void Evolucionar(unsigned int NumGeneraciones);
    void ImprimeInd(unsigned int Id);
    void ImprimePob(void);
    void Torneo(void);
    
    float GetBestObj(void) const { return POB[Id_BestObj].VObj; }
    unsigned int GetBestObj_Index(void) const { return Id_BestObj; }
    void GetBestCirculo(float *cx, float *cy, float *r) const;
    
    void setTipoFuncion(int f) { tipo_funcion = f; }
    void setDataSet(const float* x, const float* y, int n);
    void setOptDir(OPT_TYPE dir) { opt_dir = dir; }
    void setMetodoSeleccion(int met) { metodo_seleccion = met; }
    void setTournamentSize(unsigned int size) { tournament_size = size; }
    void setTipoCruza(int c) { tipo_cruza = c; }
    void setElitismo(bool val) { elitismo = val; }
    void setPixelNegros(int *x, int *y, int count,
                        float *img_data, int ancho, int alto);
};

#endif /* SGA_HPP */