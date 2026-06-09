import subprocess, csv, os, sys

BIN = "build/ga_detector.exe"
IMGS = [
    "imgs/C01.bmp", "imgs/C02.bmp", "imgs/C03.bmp",
    "imgs/C04.bmp", "imgs/C05.bmp", "imgs/auto.bmp",
    "imgs/lamp.bmp", "imgs/1uno.bmp", "imgs/2dos.bmp"
]
POB, GENS, PC, PM, SEL, CRUZA = 100, 500, 0.90, 0.05, 2, 1
ITERS = 100

todas = []
for i, img in enumerate(IMGS):
    nombre = os.path.basename(img)
    print(f"[{i+1}/{len(IMGS)}] {nombre}...", end=" ", flush=True)
    cmd = [BIN, img, str(POB), str(GENS), str(PC), str(PM), str(SEL), str(CRUZA), str(ITERS)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    stats_line = None
    for line in proc.stdout.strip().split('\n'):
        if line.startswith("STATS"):
            stats_line = line
        else:
            try:
                cx, cy, r, fit = map(float, line.split(','))
                todas.append([nombre, cx, cy, r, fit])
            except ValueError:
                continue
    if stats_line:
        parts = stats_line.split(',')
        print(f"fit_avg={float(parts[7]):.0f}")
    else:
        print("fallo")

os.makedirs("resultados", exist_ok=True)
with open("resultados/resultados_crudos.csv", "w", newline='') as f:
    w = csv.writer(f)
    w.writerow(["Imagen", "Centro_X", "Centro_Y", "Radio", "Fitness"])
    w.writerows(todas)
print("\nCSV actualizado. Resultados en outputs/")
