import subprocess
import csv
import numpy as np
import concurrent.futures
import os

def ejecutar_tarea(args):
    """Ejecuta una instancia única del algoritmo genético."""
    imagen, pob, gens, pc, pm, sel, cruza = args
    comando = ["./ga_detector", imagen, str(pob), str(gens), str(pc), str(pm), str(sel), str(cruza)]
    
    proceso = subprocess.run(comando, capture_output=True, text=True)
    salida_limpia = proceso.stdout.strip().split('\n')[-1]
    
    try:
        cx, cy, r, fit = map(float, salida_limpia.split(','))
        return [imagen, cx, cy, r, fit]
    except ValueError:
        return None # Falla silenciosa en caso de error de parseo

def generar_reporte_paralelo(imagenes, repeticiones=100):
    todas_ejecuciones = []
    
    # Configuración estática para la prueba masiva (debe venir del tuning)
    pob, gens, pc, pm, sel, cruza = 100, 500, 0.9, 0.05, 2, 1
    
    # 1. Preparar la lista total de tareas (7 imágenes * 100 repeticiones = 700 tareas)
    tareas = []
    for img in imagenes:
        for _ in range(repeticiones):
            tareas.append((img, pob, gens, pc, pm, sel, cruza))
            
    print(f"Iniciando {len(tareas)} ejecuciones en paralelo...")
    
    # 2. Ejecutar tareas distribuyéndolas en todos los núcleos disponibles
    # 2. Ejecutar tareas distribuyéndolas en todos los núcleos y rastrear progreso
    resultados_validos = []
    with concurrent.futures.ProcessPoolExecutor() as executor:
        futuros = {executor.submit(ejecutar_tarea, tarea): tarea for tarea in tareas}
        
        completados = 0
        for futuro in concurrent.futures.as_completed(futuros):
            res = futuro.result()
            if res is not None:
                resultados_validos.append(res)
            
            completados += 1
            if completados % 50 == 0 or completados == len(tareas):
                print(f"Progreso: {completados}/{len(tareas)} ejecuciones completadas...")
                
    todas_ejecuciones.extend(resultados_validos)

    # 4. Calcular e imprimir estadísticas
    for img in imagenes:
        res_img = [r for r in resultados_validos if r[0] == img]
        if not res_img:
            print(f"Sin resultados válidos para {img}")
            continue
            
        datos = np.array(res_img)[:, 1:].astype(float)
        promedios = np.mean(datos, axis=0)
        desviaciones = np.std(datos, axis=0)
        
        print(f"Resultados para {img}:")
        print(f"Centro X: {promedios[0]:.2f} ± {desviaciones[0]:.2f}")
        print(f"Centro Y: {promedios[1]:.2f} ± {desviaciones[1]:.2f}")
        print(f"Radio:    {promedios[2]:.2f} ± {desviaciones[2]:.2f}")
        print("-" * 30)

    # 5. Generar archivo de salida
    with open("resultados_crudos.csv", "w", newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Imagen", "Centro_X", "Centro_Y", "Radio", "Fitness"])
        writer.writerows(todas_ejecuciones)
        
    print("Pruebas finalizadas. Archivo 'resultados_crudos.csv' generado correctamente.")

if __name__ == "__main__":
    lista_imagenes = [
        "imgs/C01.bmp", "imgs/C02.bmp", "imgs/C03.bmp", 
        "imgs/C04.bmp", "imgs/C05.bmp", "imgs/auto.bmp", "imgs/lamp.bmp"
    ]
    generar_reporte_paralelo(lista_imagenes, repeticiones=100)