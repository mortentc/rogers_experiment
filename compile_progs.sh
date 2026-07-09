clang datagen.c -o datagen
clang -O2 -target bpf -c external/delilah/delilah/programs/rogers_programs/rogers_aggregate_by_pos.c -o programs/aggregate.o
clang -O2 -target bpf -c external/delilah/delilah/programs/aggregate_by_pos.c -o programs/old_aggregate.o
clang -O2 -target bpf -c external/delilah/delilah/programs/rogers_programs/rogers_convert.c -o programs/rogers_convert.o
clang -O2 -target bpf -c external/delilah/delilah/programs/rogers_programs/obtain_1.c -o programs/obtain_1.o
clang -O2 -target bpf -c external/delilah/delilah/programs/rogers_programs/empty.c -o programs/empty.o 
cmake --build build
cmake build
./datagen
build/experiment1
build/experiment2
build/experiment3
build/experiment4
build/experiment5