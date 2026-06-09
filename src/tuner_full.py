"""
Tuning completo multi-imagen para deteccion de circulos con AG.
Prueba 12 configuraciones en 6 imagenes representativas.
"""
import subprocess, sys, csv, math, itertools, time
import numpy as np

BIN = "build/ga_detector.exe"
IMGS = ["imgs/C01.bmp", "imgs/C03.bmp", "imgs/C05.bmp",
        "imgs/auto.bmp", "imgs/lamp.bmp", "imgs/1uno.bmp"]
ITERS = 10  # iteraciones internas por corrida

# Configs: (POB, GEN, PC, PM, SEL, CRUZA)
configs = [
    # Default
    (100, 500, 0.90, 0.05, 2, 1),
    # SEL variation
    (100, 500, 0.90, 0.05, 1, 1),
    (100, 500, 0.90, 0.05, 3, 1),
    # POB variation
    ( 50, 500, 0.90, 0.05, 2, 1),
    (200, 500, 0.90, 0.05, 2, 1),
    # PM variation
    (100, 500, 0.90, 0.01, 2, 1),
    (100, 500, 0.90, 0.10, 2, 1),
    # PC variation
    (100, 500, 0.80, 0.05, 2, 1),
    # Cruza variation
    (100, 500, 0.90, 0.05, 2, 2),
]

results = []
total = len(configs) * len(IMGS)
done = 0

print(f"Tuning {len(configs)} configs x {len(IMGS)} imagenes = {total} corridas\n")

for img in IMGS:
    short = img.split('/')[-1]
    for cfg in configs:
        pob, gen, pc, pm, sel, cruza = cfg
        cmd = [BIN, img, str(pob), str(gen), str(pc), str(pm), str(sel), str(cruza), str(ITERS)]
        t0 = time.time()
        proc = subprocess.run(cmd, capture_output=True, text=True)
        t = time.time() - t0

        # Parse last line (STATS)
        lines = proc.stdout.strip().split('\n')
        stats = [l for l in lines if l.startswith("STATS")]
        if not stats:
            done += 1
            continue
        parts = stats[-1].split(',')
        fit_avg = float(parts[7])
        fit_sd  = float(parts[8])

        done += 1
        eta = (total - done) * (t / done) / 60
        print(f"[{done}/{total}] {short} POB={pob} GEN={gen} PC={pc} PM={pm} SEL={sel} CRU={cruza} -> fit={fit_avg:.0f}+-{fit_sd:.1f} ({t:.1f}s) ETA={eta:.1f}min")

        results.append({
            'Imagen': short, 'POB': pob, 'GEN': gen, 'PC': pc, 'PM': pm,
            'SEL': sel, 'CRUZA': cruza, 'Fit_avg': fit_avg, 'Fit_sd': fit_sd
        })

# Save CSV
with open('../resultados/tuning_multimagen.csv', 'w', newline='') as f:
    w = csv.DictWriter(f, fieldnames=results[0].keys())
    w.writeheader()
    w.writerows(results)

print("\n=== RESUMEN ===")
# Group by config, average across images
by_cfg = {}
for r in results:
    key = (r['POB'], r['GEN'], r['PC'], r['PM'], r['SEL'], r['CRUZA'])
    if key not in by_cfg:
        by_cfg[key] = []
    by_cfg[key].append(r['Fit_avg'])

print(f"{'POB':>4} {'GEN':>4} {'PC':>4} {'PM':>5} {'SEL':>3} {'CRU':>3} | {'Avg_Fit':>7} | {'Min':>5} {'Max':>5}")
print("-" * 60)
for key, fits in sorted(by_cfg.items()):
    print(f"{key[0]:>4} {key[1]:>4} {key[2]:>4.2f} {key[3]:>5.2f} {key[4]:>3} {key[5]:>3} | {np.mean(fits):>7.1f} | {np.min(fits):>5.0f} {np.max(fits):>5.0f}")

# Per-image summary
print(f"\n{'Imagen':>10} ", end='')
for c in range(len(configs)):
    print(f"  C{c:02d}", end='')
print(f" | {'Avg':>5}")
print("-" * (10 + 6*len(configs) + 8))
for img in [x.split('/')[-1] for x in IMGS]:
    img_fits = [r['Fit_avg'] for r in results if r['Imagen'] == img]
    print(f"{img:>10} ", end='')
    for f in img_fits:
        print(f" {f:>5.0f}", end='')
    print(f" | {np.mean(img_fits):>5.0f}")
