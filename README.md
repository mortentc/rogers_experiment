# Evaluation of Rogers framework for computational storage
- `compile_progs.sh` compiles both the CSFs and experiments, runs the experiments and prints the results to termial. Use `cmake -B build` before running. Results are also written to `data/`.
- `external/` contains a reference to the Delilah version of the [UBPF VM](https://github.com/iovisor/ubpf) which is used as execution environment in some of the experiments. Make sure to initialize submodules if you intend to run the experiments.
- `functions/` contain a mix of Delilah and Norville CSFs. Not all CSFs are used in the experiments.
- `functions/verified_funcs` contains registered functions verified with [Frama-C](https://frama-c.com/)
- `program/` is a directory for `compile_progs.sh` to store the eBPF versions of the executed CSFs.
