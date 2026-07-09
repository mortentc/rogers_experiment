clang datagen.c -o datagen
clang -O2 -target bpf -c functions/rogers_aggregate_by_pos.c -o programs/aggregate.o
clang -O2 -target bpf -c functions/aggregate_by_pos.c -o programs/old_aggregate.o
clang -O2 -target bpf -c functions/rogers_convert.c -o programs/rogers_convert.o
clang -O2 -target bpf -c functions/obtain_1.c -o programs/obtain_1.o
clang -O2 -target bpf -c functions/empty.c -o programs/empty.o 
cmake --build build
cmake build
./datagen
build/experiment1
build/experiment2
build/experiment3
build/experiment4
build/experiment5