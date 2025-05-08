clang -O2 -target bpf -c external/delilah/delilah/programs/roger_programs/roger_aggregate_by_pos.c -o programs/aggregate.o
clang -O2 -target bpf -c programs/test.c -o programs/test.o