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
    lineas = proceso.stdout.strip().split('\n')
    
    resultados_crudos = []
    estadisticas = None
    
    for linea in lineas:
        if linea.startswith("STATS"):
            # Capturamos las estadísticas descriptivas calculadas en C++
            _, cx_avg, cx_sd, cy_avg, cy_sd, r_avg, r_sd, fit_avg, fit_sd = linea.split(',')
            estadisticas = [float(x) for x in (cx_avg, cx_sd, cy_avg, cy_sd, r_avg, r_sd, fit_avg, fit_sd)]
        else:
            try:
                cx, cy, r, fit = map(float, linea.split(','))
                resultados_crudos.append([imagen, cx, cy, r, fit])
            except ValueError:
                continue
                
    return imagen, resultados_crudos, estadisticas

def generar_reporte_paralelo(imagenes):
    todas_ejecuciones = []
    resultados_agregados = {}
    
    pob, gens, pc, pm, sel, cruza = 100, 500, 0.90, 0.05, 2, 1
    
    # 1 tarea por imagen (C++ hará las 100 iteraciones internamente)
    tareas = [(img, pob, gens, pc, pm, sel, cruza) for img in imagenes]
        
    print(f"Iniciando ejecución en paralelo para {len(tareas)} imágenes...")
    
    with concurrent.futures.ProcessPoolExecutor() as executor:
        futuros = {executor.submit(ejecutar_tarea, tarea): tarea for tarea in tareas}
        
        completados = 0
        for futuro in concurrent.futures.as_completed(futuros):
            imagen, res_crudos, stats = futuro.result()
            if res_crudos:
                todas_ejecuciones.extend(res_crudos)
            if stats:
                resultados_agregados[imagen] = stats
            
            completados += 1
            print(f"Progreso: {completados}/{len(tareas)} imágenes procesadas...")

    # Generar archivo CSV con las 700 ejecuciones
    with open("resultados_crudos.csv", "w", newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Imagen", "Centro_X", "Centro_Y", "Radio", "Fitness"])
        writer.writerows(todas_ejecuciones)
        
    print("\nArchivo 'resultados_crudos.csv' generado correctamente.\n")

    # Imprimir tabla ASCII con las estadísticas
    ancho_img = 10
    print(f"+{'-'*(ancho_img+2)}+{'-'*14}+{'-'*13}+{'-'*14}+{'-'*13}+{'-'*10}+{'-'*10}+{'-'*12}+{'-'*11}+")
    print(f"| {'Imagen':<{ancho_img}} | {'Centro X avg':<12} | {'Centro X sd':<11} | {'Centro Y avg':<12} | {'Centro Y sd':<11} | {'R avg':<8} | {'R sd':<8} | {'Fit avg':<10} | {'Fit sd':<9} |")
    print(f"+{'-'*(ancho_img+2)}+{'-'*14}+{'-'*13}+{'-'*14}+{'-'*13}+{'-'*10}+{'-'*10}+{'-'*12}+{'-'*11}+")

    for img in imagenes:
        nombre_corto = img.split('/')[-1]
        if img in resultados_agregados:
            cx_avg, cx_sd, cy_avg, cy_sd, r_avg, r_sd, fit_avg, fit_sd = resultados_agregados[img]
            print(f"| {nombre_corto:<{ancho_img}} | {cx_avg:>12.2f} | {cx_sd:>11.2f} | {cy_avg:>12.2f} | {cy_sd:>11.2f} | {r_avg:>8.2f} | {r_sd:>8.2f} | {fit_avg:>10.0f} | {fit_sd:>9.2f} |")
        else:
            print(f"| {nombre_corto:<{ancho_img}} | {'Sin datos suficientes calculados':<101} |")
            
    print(f"+{'-'*(ancho_img+2)}+{'-'*14}+{'-'*13}+{'-'*14}+{'-'*13}+{'-'*10}+{'-'*10}+{'-'*12}+{'-'*11}+")

if __name__ == "__main__":
    lista_imagenes = [
        "imgs/C01.bmp", "imgs/C02.bmp", "imgs/C03.bmp", 
        "imgs/C04.bmp", "imgs/C05.bmp", "imgs/auto.bmp", "imgs/lamp.bmp"
    ]
    generar_reporte_paralelo(lista_imagenes)