import subprocess
import numpy as np

DEFAULT = dict(POB=100, GEN=500, PC=0.90, PM=0.05, SEL=2)

combos = []

# SEL variation (1=ruleta, 2=torneo2, 3=torneo3, 4=torneo4)
for sel in [1, 2, 3, 4]:
    c = DEFAULT.copy()
    c['SEL'] = sel
    combos.append(c)

# POB variation
for pob in [50, 100, 200]:
    c = DEFAULT.copy()
    c['POB'] = pob
    combos.append(c)

# PM variation
for pm in [0.01, 0.05, 0.10]:
    c = DEFAULT.copy()
    c['PM'] = pm
    combos.append(c)

# PC variation
for pc in [0.80, 0.90]:
    c = DEFAULT.copy()
    c['PC'] = pc
    combos.append(c)

print(f"{'POB':>4} {'GEN':>4} {'PC':>4} {'PM':>5} {'SEL':>3} | {'Fit_avg':>7} {'Fit_sd':>6} {'Cx_avg':>7} {'Cy_avg':>7} {'R_avg':>6}")
print("-" * 65)

for c in combos:
    cmd = ["build/ga_detector.exe", "imgs/C01.bmp",
           str(c['POB']), str(c['GEN']), str(c['PC']), str(c['PM']), str(c['SEL']), "1"]
    proc = subprocess.run(cmd + ["10"], capture_output=True, text=True)
    stats_line = proc.stdout.strip().split('\n')[-1]
    # STATS,cx_avg,cx_sd,cy_avg,cy_sd,r_avg,r_sd,fit_avg,fit_sd
    parts = stats_line.split(',')
    cx_avg, cx_sd = float(parts[1]), float(parts[2])
    cy_avg, cy_sd = float(parts[3]), float(parts[4])
    r_avg, r_sd = float(parts[5]), float(parts[6])
    fit_avg, fit_sd = float(parts[7]), float(parts[8])

    print(f"{c['POB']:>4} {c['GEN']:>4} {c['PC']:>4.2f} {c['PM']:>5.2f} {c['SEL']:>3} | {fit_avg:>7.1f} {fit_sd:>6.2f} {cx_avg:>7.2f} {cy_avg:>7.2f} {r_avg:>6.2f}")
