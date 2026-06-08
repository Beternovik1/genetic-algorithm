import itertools
import subprocess
import numpy as np
import pandas as pd

def sintonizar_parametros(imagen="imgs/C01.bmp"):
    poblaciones = [50, 100, 200, 500]
    mutaciones = [0.01, 0.05, 0.1, 0.2]
    cruzas = [0.5, 0.7, 0.9]
    generaciones = [100, 200, 500, 1000]
    selecciones = [1, 2] # 1: Ruleta, 2: Torneo
    
    combinaciones = list(itertools.product(poblaciones, mutaciones, cruzas, generaciones, selecciones))
    resultados_tuning = []
    
    for combo in combinaciones:
        pob, pm, pc, gen, sel = combo
        comando = ["../build/ga_detector.exe", imagen, str(pob), str(gen), str(pc), str(pm), str(sel), "1"]
        
        # Ejecutar 5 veces por configuración para obtener un promedio rápido (100 tomaría mucho tiempo en el tuning)
        fits = []
        for _ in range(5):
            proc = subprocess.run(comando, capture_output=True, text=True)
            salida = proc.stdout.strip().split('\n')[-1]
            fits.append(float(salida.split(',')[3]))
            
        avg_fit = np.mean(fits)
        resultados_tuning.append({"POB": pob, "PM": pm, "PC": pc, "GEN": gen, "SEL": sel, "Avg_Fit": avg_fit})
        
    df = pd.DataFrame(resultados_tuning)
    df.to_csv("../resultados/tabla_configuraciones.csv", index=False)
    
    mejor = df.loc[df['Avg_Fit'].idxmax()]
    print("La mejor configuración encontrada es:")
    print(mejor)

if __name__ == "__main__":
    sintonizar_parametros()