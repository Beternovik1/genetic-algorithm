import subprocess, os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

BIN = "build/ga_detector.exe"
IMGS = [
    "imgs/C01.bmp", "imgs/C02.bmp", "imgs/C03.bmp",
    "imgs/C04.bmp", "imgs/C05.bmp", "imgs/auto.bmp",
    "imgs/lamp.bmp", "imgs/1uno.bmp", "imgs/2dos.bmp"
]
POB, PC, PM, SEL, CRUZA = 100, 0.90, 0.05, 2, 1

def get_fit(img_path, gens, iters=15):
    cmd = [BIN, img_path, str(POB), str(gens), str(PC), str(PM), str(SEL), str(CRUZA), str(iters)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    for line in proc.stdout.strip().split('\n'):
        if line.startswith("STATS"):
            return float(line.split(',')[7])
    return None

GEN_SAMPLES = [10, 25, 50, 75, 100, 150, 200, 300, 400, 500]
convergencia = {os.path.basename(img).replace('.bmp',''): [] for img in IMGS}

for gens in GEN_SAMPLES:
    print(f"gens={gens}...", end=" ", flush=True)
    for img in IMGS:
        nom = os.path.basename(img).replace('.bmp','')
        fit = get_fit(img, gens)
        if fit is not None:
            convergencia[nom].append((gens, fit))
    print("ok")

fig, ax = plt.subplots(figsize=(10, 5))
for nom_corto, puntos in convergencia.items():
    if not puntos:
        continue
    gs = [p[0] for p in puntos]
    fits = [p[1] for p in puntos]
    ax.plot(gs, fits, marker='o', markersize=3, lw=1.5, label=nom_corto)
ax.set_xlabel("Generaciones")
ax.set_ylabel("Fitness promedio")
ax.set_title("Curva de convergencia real del fitness")
ax.legend(fontsize=8, ncol=3)
ax.grid(alpha=0.3)
plt.tight_layout()
plt.savefig("outputs/convergencia_simulada.png", dpi=150)
plt.close()
print("\noutputs/convergencia_simulada.png")
