import matplotlib.pyplot as plt
import numpy as np
# plt.style.use('_mpl-gallery')
fig, ax = plt.subplots()
plt.ylabel("Time taken [ms]")
plt.yscale("log")

indices = 10**6 / 2
runs = 10

with open("data/results.txt") as results:
    # ys = list(map(lambda y: round(indices*8/float(y)/10**3), results.readlines()))
    ys = list(map(float, results.readlines()))
    yss = [ys[:runs], ys[runs:2*runs], ys[2*runs:3*runs], ys[3*runs:]]
    ya = np.array(yss)
    means = ya.mean(1)
    mins = ya.min(1)
    maxes = ya.max(1)
    std = ya.std(1)

    # print(ya)
    xs = ["Mixed Verified", "Compiled Verified", "VM Original", "Compiled Original"]
    plt.errorbar(xs, means, [means - mins, maxes-means], fmt=".k", ecolor="gray", lw=1)
    plt.errorbar(xs, means, std, fmt="ok", lw=3)
    # plt.bar_label(label_type="edge")
    # p = ax.bar(xs, ya.mean(1))
    # ax.bar_label(p, label_type="center")
    # ax.set_title("Aggregate by Position")

plt.savefig("data/results_dev.png")