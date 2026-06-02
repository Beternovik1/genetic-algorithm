#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include "sga.hpp"

using namespace std;

// Función para guardar los resultados en un archivo CSV
void guardarResultados(const string& nombreArchivo, float resultados[], int n) {
    ofstream archivo(nombreArchivo);
    if (archivo.is_open()) {
        archivo << "Resultado\n";
        for (int i = 0; i < n; i++) {
            archivo << resultados[i] << "\n";
        }
        archivo.close();
    } else {
        cerr << "Error al abrir el archivo " << nombreArchivo << endl;
    }
}

int main() {
    time_t tx;
    srand((unsigned)time(&tx));

    const unsigned int NUM_GENES = 10;
    unsigned int NumBITsxGEN[NUM_GENES];
    float ls[NUM_GENES];
    float li[NUM_GENES];

    // Configuración: 16 bits de resolución y rango [-20, 20] [cite: 14]
    for(int i = 0; i < NUM_GENES; i++) {
        NumBITsxGEN[i] = 16;
        ls[i] = 20.0f;
        li[i] = -20.0f;
    }

    const unsigned int TAM_POBLACION = 100;  
    const unsigned int NUM_GENERACIONES = 200; 
    const int EJECUCIONES = 100;

    // Configuración de los operadores de cruza: 1 = 1 Punto, 2 = 2 Puntos, 3 = Uniforme
    int tipos_cruza[] = {1, 2, 3};
    string nombres_cruza[] = {"1_Punto", "2_Puntos", "Uniforme"};
    float resultados[EJECUCIONES];

    for (int c = 0; c < 3; c++) {
        int tc = tipos_cruza[c];
        cout << "Ejecutando " << EJECUCIONES << " veces con Cruza: " << nombres_cruza[c] << "..." << endl;

        for (int i = 0; i < EJECUCIONES; i++) {
            // Inicialización con los parámetros fijos de sintonización
            GA ga(TAM_POBLACION, NUM_GENES, NumBITsxGEN, ls, li, 0.90f, 0.01f);
            ga.setTipoFuncion(4); // Función objetivo de la tarea [cite: 12]
            ga.setOptDir(MAX);
            
            // Selección fija para mantener el entorno controlado [cite: 17]
            ga.setMetodoSeleccion(2); // 2 = Torneo
            ga.setTournamentSize(2);  
            
            // Configurar el operador de cruza correspondiente
            ga.setTipoCruza(tc); 

            ga.Evolucionar(NUM_GENERACIONES);
            resultados[i] = ga.GetBestObj();
        }

        // Almacenamiento individual por operador
        string nombre_archivo = "resultados_Cruza_" + nombres_cruza[c] + ".csv";
        guardarResultados(nombre_archivo, resultados, EJECUCIONES);
    }

    cout << "Archivos CSV generados exitosamente." << endl;
    
    return 0;
}