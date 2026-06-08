/* sga.cpp
   Implementación del Algoritmo Genético Simple
   Modificado para detección de círculos en imágenes BMP
 */

#include "sga.hpp"
#define PI 3.14159265358979323846f

/* Función auxiliar para calcular el círculo que pasa por 3 puntos */
static int _calcular_circulo_3puntos(int x1, int y1, int x2, int y2,
                                     int x3, int y3, float *cx, float *cy, float *r)
{
    float a = (float)(x1 - x2);
    float b = (float)(y1 - y2);
    float c = (float)(x1 - x3);
    float d = (float)(y1 - y3);
    float e = (float)((x1*x1 - x2*x2) + (y1*y1 - y2*y2)) / 2.0f;
    float f = (float)((x1*x1 - x3*x3) + (y1*y1 - y3*y3)) / 2.0f;
    float det = a * d - b * c;

    if (fabs(det) < 1e-6f)
        return 0; /* Puntos colineales */

    *cx = (d * e - b * f) / det;
    *cy = (-c * e + a * f) / det;
    *r  = sqrtf((x1 - *cx) * (x1 - *cx) + (y1 - *cy) * (y1 - *cy));
    return 1;
}

void GA::AllocIndividuo(INDIVIDUO &ind) {
    ind.Chrom = new BYTE[ChromeSize];
    ind.Vent  = new unsigned int[NumGens];
    ind.Vre   = new float[NumGens];
    ind.VObj  = 0.0f;
    ind.VFit  = 0.0f;
    for (unsigned int i = 0; i < ChromeSize; i++) ind.Chrom[i] = 0;
    for (unsigned int j = 0; j < NumGens;    j++) { ind.Vent[j] = 0; ind.Vre[j] = 0.0f; }
}

GA::GA(unsigned int T_Pob,
       unsigned int N_Gens,
       const unsigned int *NBsxGen,
       const float        *L_S,
       const float        *L_I,
       float               ProbC,
       float               ProbM)
{
    PobSize      = T_Pob;
    NumGens      = N_Gens;
    BitxGen      = NBsxGen;
    LSup         = L_S;
    LInf         = L_I;
    ProbCruza    = ProbC;
    ProbMutacion = ProbM;
    ChromeSize   = 0;
    opt_dir      = MIN;
    metodo_seleccion = 1;
    tournament_size = 2;
    tipo_cruza = 1;
    elitismo   = true;
    pixel_x     = NULL;
    pixel_y     = NULL;
    cant_pixeles_negros = 0;
    imagen_plana = NULL;
    ancho_img   = 0;
    alto_img    = 0;

    for (unsigned int k = 0; k < NumGens; k++)
        ChromeSize += BitxGen[k];

    POB = new INDIVIDUO[PobSize];
    for (unsigned int k = 0; k < PobSize; k++) {
        AllocIndividuo(POB[k]);
        for (unsigned int i = 0; i < ChromeSize; i++)
            POB[k].Chrom[i] = rand() % 2;
    }

    TmpPOB = new INDIVIDUO[PobSize];
    for (unsigned int k = 0; k < PobSize; k++)
        AllocIndividuo(TmpPOB[k]);

    SelPool    = new unsigned int[PobSize];
    EliteChrom = new BYTE[ChromeSize];
    for (unsigned int i = 0; i < ChromeSize; i++) EliteChrom[i] = 0;

    Id_BestObj = Id_WorstObj = 0;
    SumObj = PromObj = SumFit = PromFit = 0.0f;
}

GA::~GA() {
    for (unsigned int k = 0; k < PobSize; k++) {
        delete[] POB[k].Chrom;
        delete[] POB[k].Vent;
        delete[] POB[k].Vre;
        delete[] TmpPOB[k].Chrom;
        delete[] TmpPOB[k].Vent;
        delete[] TmpPOB[k].Vre;
    }
    delete[] POB;
    delete[] TmpPOB;
    delete[] SelPool;
    delete[] EliteChrom;
}

void GA::DecodeEnt(void) {
    for (unsigned int k = 0; k < PobSize; k++) {
        unsigned int Acumulado = BitxGen[0];
        unsigned int g   = 0;
        unsigned int aux = 0;

        for (unsigned int i = 0, j = 0; i < ChromeSize; i++, j++) {
            aux += POB[k].Chrom[i] * (unsigned int)pow(2.0, (double)j);

            if (i == (Acumulado - 1)) {
                POB[k].Vent[g] = aux;
                aux = 0;
                g++;
                if (g < NumGens)          
                    Acumulado += BitxGen[g];
                j = (unsigned int)-1;     
            }
        }
    }
}

void GA::DecodeReal(void) {
    for (unsigned int k = 0; k < PobSize; k++) {
        for (unsigned int g = 0; g < NumGens; g++) {
            float rango = LSup[g] - LInf[g];
            float Den   = (float)(pow(2.0, (double)BitxGen[g]) - 1.0);
            POB[k].Vre[g] = ((POB[k].Vent[g] / Den) * rango) + LInf[g];
        }
    }
}

void GA::Roulette(void) {
    float *CumFit = new float[PobSize];
    CumFit[0] = POB[0].VFit;
    for (unsigned int k = 1; k < PobSize; k++)
        CumFit[k] = CumFit[k - 1] + POB[k].VFit;

    float Total = CumFit[PobSize - 1];

    for (unsigned int k = 0; k < PobSize; k++) {
        float r   = ((float)rand() / (float)RAND_MAX) * Total;
        unsigned int sel = 0;
        while (sel < PobSize - 1 && r > CumFit[sel])
            sel++;
        SelPool[k] = sel;
    }
    delete[] CumFit;
}

void GA::Torneo(void) {
    for (unsigned int k = 0; k < PobSize; k++) {
        // Selecciona el primer participante al azar
        unsigned int best_idx = rand() % PobSize;
        
        // Compiten N-1 participantes adicionales
        for (unsigned int i = 1; i < tournament_size; i++) {
            unsigned int competitor = rand() % PobSize;
            if (POB[competitor].VFit > POB[best_idx].VFit) {
                best_idx = competitor;
            }
        }
        SelPool[k] = best_idx;
    }
}

void GA::Cruza(void) {
    unsigned int pares = PobSize / 2;

    for (unsigned int k = 0; k < pares; k++) {
        unsigned int p1 = SelPool[2 * k];
        unsigned int p2 = SelPool[2 * k + 1];

        // Copia de padres a hijos (por defecto si no hay cruza)
        for (unsigned int i = 0; i < ChromeSize; i++) {
            TmpPOB[2 * k].Chrom[i]     = POB[p1].Chrom[i];
            TmpPOB[2 * k + 1].Chrom[i] = POB[p2].Chrom[i];
        }

        float r = (float)rand() / (float)RAND_MAX;
        
        // Aplicar operador de cruza según probabilidad
        if (r < ProbCruza) {
            if (tipo_cruza == 1) { 
                // Cruza de 1 Punto
                unsigned int punto = rand() % (ChromeSize - 1) + 1;
                for (unsigned int i = punto; i < ChromeSize; i++) {
                    TmpPOB[2 * k].Chrom[i]     = POB[p2].Chrom[i];
                    TmpPOB[2 * k + 1].Chrom[i] = POB[p1].Chrom[i];
                }
            } 
            else if (tipo_cruza == 2) { 
                // Cruza de 2 Puntos
                unsigned int p_a = rand() % (ChromeSize - 1) + 1;
                unsigned int p_b = rand() % (ChromeSize - 1) + 1;
                if (p_a > p_b) {
                    unsigned int tmp = p_a;
                    p_a = p_b;
                    p_b = tmp;
                }
                for (unsigned int i = p_a; i < p_b; i++) {
                    TmpPOB[2 * k].Chrom[i]     = POB[p2].Chrom[i];
                    TmpPOB[2 * k + 1].Chrom[i] = POB[p1].Chrom[i];
                }
            } 
            else if (tipo_cruza == 3) { 
                // Cruza Uniforme
                for (unsigned int i = 0; i < ChromeSize; i++) {
                    if (rand() % 2 == 1) { // 50% de probabilidad de intercambiar cada bit
                        TmpPOB[2 * k].Chrom[i]     = POB[p2].Chrom[i];
                        TmpPOB[2 * k + 1].Chrom[i] = POB[p1].Chrom[i];
                    }
                }
            }
        }
    }

    // Conservar al último individuo si la población es impar
    if (PobSize % 2 != 0) {
        unsigned int last = SelPool[PobSize - 1];
        for (unsigned int i = 0; i < ChromeSize; i++)
            TmpPOB[PobSize - 1].Chrom[i] = POB[last].Chrom[i];
    }

    // Actualizar población
    for (unsigned int k = 0; k < PobSize; k++)
        for (unsigned int i = 0; i < ChromeSize; i++)
            POB[k].Chrom[i] = TmpPOB[k].Chrom[i];
}

void GA::Mutacion(void) {
    for (unsigned int k = 0; k < PobSize; k++) {
        for (unsigned int i = 0; i < ChromeSize; i++) {
            float r = (float)rand() / (float)RAND_MAX;
            if (r < ProbMutacion)
                POB[k].Chrom[i] = 1 - POB[k].Chrom[i];
        }
    }
}

void GA::Evolucionar(unsigned int NumGeneraciones) {
    float globalBestObj = (opt_dir == MIN) ? 1e30f : -1e30f;
    bool  eliteValido   = false;

    for (unsigned int gen = 0; gen < NumGeneraciones; gen++) {
        DecodeEnt(); DecodeReal(); EvaluarPob();

        bool esMejor = false;
        if (opt_dir == MIN && POB[Id_BestObj].VObj < globalBestObj) esMejor = true;
        if (opt_dir == MAX && POB[Id_BestObj].VObj > globalBestObj) esMejor = true;

        if (esMejor) {
            globalBestObj = POB[Id_BestObj].VObj;
            for (unsigned int i = 0; i < ChromeSize; i++)
                EliteChrom[i] = POB[Id_BestObj].Chrom[i];
            eliteValido = true;
        }

        Obj_to_Fit(opt_dir); 
        
        if (metodo_seleccion == 1) Roulette();
        else Torneo();
        
        Cruza(); Mutacion();

        if (elitismo && eliteValido)
            for (unsigned int i = 0; i < ChromeSize; i++)
                POB[0].Chrom[i] = EliteChrom[i];
    }
}

float GA::FuncionObjetivo(unsigned int Id) {
    float obj = 0.0f;
    float pi = 3.141592653589793f;

    if (tipo_funcion == 1) { 
        for (unsigned int i = 0; i < NumGens; i++) obj += POB[Id].Vre[i] * POB[Id].Vre[i];
    } 
    else if (tipo_funcion == 2) { 
        float suma = 0.0f, prod = 1.0f;
        for (unsigned int i = 0; i < NumGens; i++) {
            float val = std::abs(POB[Id].Vre[i]);
            suma += val; prod *= val;
        }
        obj = suma + prod;
    } 
    else if (tipo_funcion == 9) { 
        for (unsigned int i = 0; i < NumGens; i++) {
            float xi = POB[Id].Vre[i];
            obj += (xi * xi - 10.0f * std::cos(2.0f * pi * xi) + 10.0f);
        }
    }
    else if (tipo_funcion == 3) { 
        float mse = 0.0f;
        int m = NumGens / 3; 

        for (int i = 0; i < num_puntos; i++) {
            float f_x = 0.0f;
            for (int j = 0; j < m; j++) {
                float lambda = POB[Id].Vre[j];
                float c = POB[Id].Vre[m + j];
                float s = POB[Id].Vre[2 * m + j];
                if (s == 0.0f) s = 1e-5f; 
                float exponente = -((X_data[i] - c) * (X_data[i] - c)) / (2.0f * s * s);
                f_x += lambda * std::exp(exponente);
            }
            float error = f_x - Y_data[i];
            mse += error * error;
        }
        obj = mse / (float)num_puntos;
    }
    else if (tipo_funcion == 4) { // Tarea 03
        obj = 1000.0f;
        float centros[10] = {-7.5f, -3.0f, 3.0f, 5.0f, -2.5f, 10.0f, 15.0f, -10.0f, -15.0f, 0.5f};
        for(int i = 0; i < 10; i++) {
            float diff = POB[Id].Vre[i] - centros[i];
            obj -= (diff * diff);
        }
    }
    else if (tipo_funcion == 5) {
        /* Detección de círculos en imágenes BMP
           Vre[0] = I (índice al primer  píxel negro)
           Vre[1] = J (índice al segundo píxel negro)
           Vre[2] = K (índice al tercer  píxel negro)
         */
        int i_idx = (int)POB[Id].Vre[0];
        int j_idx = (int)POB[Id].Vre[1];
        int k_idx = (int)POB[Id].Vre[2];

        /* Validar que los índices estén dentro del rango */
        if (i_idx < 0 || i_idx >= cant_pixeles_negros ||
            j_idx < 0 || j_idx >= cant_pixeles_negros ||
            k_idx < 0 || k_idx >= cant_pixeles_negros) {
            return 0.0f;
        }

        /* Obtener coordenadas de los 3 puntos */
        int x1 = pixel_x[i_idx], y1 = pixel_y[i_idx];
        int x2 = pixel_x[j_idx], y2 = pixel_y[j_idx];
        int x3 = pixel_x[k_idx], y3 = pixel_y[k_idx];

        /* Calcular el círculo que pasa por los 3 puntos */
        float cx, cy, r;
        if (!_calcular_circulo_3puntos(x1, y1, x2, y2, x3, y3, &cx, &cy, &r)) {
            return 0.0f; /* Puntos colineales */
        }

        /* Evaluar cuántos píxeles de la circunferencia son negros */
        obj = (float)funcion_objetivo_circulo((int)cx, (int)cy, (int)r);
    }
    return obj;
}
  
void GA::EvaluarPob(void) {
    Id_BestObj  = 0;
    Id_WorstObj = 0;
    SumObj      = 0.0f;

    for (unsigned int k = 0; k < PobSize; k++) {
        POB[k].VObj = FuncionObjetivo(k);

        if (opt_dir == MIN) {
            if (POB[k].VObj < POB[Id_BestObj].VObj)  Id_BestObj  = k;
            if (POB[k].VObj > POB[Id_WorstObj].VObj) Id_WorstObj = k;
        } else { // MAX
            if (POB[k].VObj > POB[Id_BestObj].VObj)  Id_BestObj  = k;
            if (POB[k].VObj < POB[Id_WorstObj].VObj) Id_WorstObj = k;
        }
        SumObj += POB[k].VObj;
    }
    PromObj = SumObj / (float)PobSize;
}

void GA::Obj_to_Fit(OPT_TYPE Tipo) {
    unsigned int k;
    SumFit = 0.0f;

    if (Tipo == MAX) {
        float rango = POB[Id_BestObj].VObj - POB[Id_WorstObj].VObj;
        if (rango == 0.0f) {
            for (k = 0; k < PobSize; k++) POB[k].VFit = 100.0f;
            SumFit  = 100.0f * PobSize;
            PromFit = 100.0f;
            return;
        }
        for (k = 0; k < PobSize; k++) {
            POB[k].VFit = 100.0f * ((POB[k].VObj - POB[Id_WorstObj].VObj) / rango);
            SumFit += POB[k].VFit;
        }
    } else {
        for (k = 0; k < PobSize; k++) {
            POB[k].VFit = POB[Id_WorstObj].VObj - POB[k].VObj;
        }
        float minFit = POB[Id_WorstObj].VFit; 
        float rango = POB[Id_BestObj].VFit - minFit;

        if (rango == 0.0f) {
            for (k = 0; k < PobSize; k++) POB[k].VFit = 100.0f;
            SumFit  = 100.0f * PobSize;
            PromFit = 100.0f;
            return;
        }
        for (k = 0; k < PobSize; k++) {
            POB[k].VFit = 100.0f * (POB[k].VFit / rango);
            SumFit += POB[k].VFit;
        }
    }
    PromFit = SumFit / (float)PobSize;
}

void GA::ImprimeInd(unsigned int Id) {
    cout << "[" << Id << "] Obj: " << POB[Id].VObj << " Fit: " << POB[Id].VFit << endl;
}

void GA::ImprimePob(void) {
    for (unsigned int k = 0; k < PobSize; k++) ImprimeInd(k);
}

void GA::setDataSet(const float* x, const float* y, int n) {
    X_data = x; Y_data = y; num_puntos = n;
}

void GA::setPixelNegros(int *x, int *y, int count,
                         float *img_data, int ancho, int alto)
{
    pixel_x = x;
    pixel_y = y;
    cant_pixeles_negros = count;
    imagen_plana = img_data;
    ancho_img = ancho;
    alto_img  = alto;
}

int GA::funcion_objetivo_circulo(int cx, int cy, int r)
{
    // Tabla estática precalculada para evitar cálculos trigonométricos redundantes
    static float cos_lut[360];
    static float sin_lut[360];
    static bool lut_init = false;

    if (!lut_init) {
        for(int i = 0; i < 360; i++) {
            float rad = (float)i * PI / 180.0f;
            cos_lut[i] = cosf(rad);
            sin_lut[i] = sinf(rad);
        }
        lut_init = true;
    }

    int aciertos = 0;
    for (int angulo = 0; angulo < 360; angulo++) {
        int px = (int)((float)cx + (float)r * cos_lut[angulo]);
        int py = (int)((float)cy + (float)r * sin_lut[angulo]);

        if (px >= 0 && px < ancho_img && py >= 0 && py < alto_img) {
            if (imagen_plana[py * ancho_img + px] == 0.0f)
                aciertos++;
        }
    }
    return aciertos;
}
void GA::GetBestCirculo(float *cx, float *cy, float *r) const {
    unsigned int idx = Id_BestObj;
    int i_idx = (int)POB[idx].Vre[0];
    int j_idx = (int)POB[idx].Vre[1];
    int k_idx = (int)POB[idx].Vre[2];

    if (i_idx < 0 || i_idx >= cant_pixeles_negros ||
        j_idx < 0 || j_idx >= cant_pixeles_negros ||
        k_idx < 0 || k_idx >= cant_pixeles_negros) {
        *cx = *cy = *r = 0.0f;
        return;
    }

    int x1 = pixel_x[i_idx], y1 = pixel_y[i_idx];
    int x2 = pixel_x[j_idx], y2 = pixel_y[j_idx];
    int x3 = pixel_x[k_idx], y3 = pixel_y[k_idx];

    _calcular_circulo_3puntos(x1, y1, x2, y2, x3, y3, cx, cy, r);
}