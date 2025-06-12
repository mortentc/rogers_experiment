import matplotlib.pyplot as plt
import numpy as np
# plt.style.use('_mpl-gallery')
fig, ax = plt.subplots()
plt.ylabel("Throughput [MB/s]")

indices = 10**8 / 2

with open("data/results.txt") as results:
    ys = list(map(lambda y: round(indices*8/float(y)/10**3), results.readlines()))
    xs = ["Mixed Verified", "Compiled Verified", "VM Original", "Compiled Original"]
    p = ax.bar(xs, ys)
    ax.bar_label(p, label_type="center")
    ax.set_title("Aggregate by Position")

plt.savefig("data/results.png")