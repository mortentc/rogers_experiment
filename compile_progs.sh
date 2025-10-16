clang datagen.c -o datagen
clang -O2 -target bpf -c external/delilah/delilah/programs/roger_programs/roger_aggregate_by_pos.c -o programs/aggregate.o
clang -O2 -target bpf -c external/delilah/delilah/programs/aggregate_by_pos.c -o programs/old_aggregate.o
cmake --build build
./datagen
build/rogers_experiment