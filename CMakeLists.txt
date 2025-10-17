cmake_minimum_required(VERSION 3.12.2)
project(rogers_experiment VERSION 0.1.0 LANGUAGES C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


add_executable(rogers_experiment main.c)
add_executable(experiment1 experiment1.c)
add_executable(experiment2 experiment2.c)
add_executable(experiment3 experiment3.c)
add_executable(experiment4 experiment4.c)
add_executable(experiment5 experiment5.c)
# add_subdirectory(external/delilah)
add_subdirectory(external/ubpf/vm)
# include_directories(external/delilah/include)
include_directories(external/ubpf/vm/inc)

file(GLOB VM_SOURCE "external/ubpf/vm/*.c")
file(GLOB VERIF_FUNCS "external/delilah/delilah/programs/verified_funcs/*.c")

add_library(vm ${VM_SOURCE})
add_library(funcs ${VERIF_FUNCS})
add_library(manager "resource_manager.c")
target_link_libraries(rogers_experiment PRIVATE vm funcs manager)
target_link_libraries(rogers_experiment PRIVATE Threads::Threads)
target_include_directories(rogers_experiment PRIVATE external/delilah/include external/delilah/delilah)

target_link_libraries(experiment1 PRIVATE vm funcs manager)
target_link_libraries(experiment1 PRIVATE Threads::Threads)
target_include_directories(experiment1 PRIVATE external/delilah/include external/delilah/delilah)

target_link_libraries(experiment2 PRIVATE vm funcs manager)
target_link_libraries(experiment2 PRIVATE Threads::Threads)
target_include_directories(experiment2 PRIVATE external/delilah/include external/delilah/delilah)

target_link_libraries(experiment3 PRIVATE vm funcs manager)
target_link_libraries(experiment3 PRIVATE Threads::Threads)
target_include_directories(experiment3 PRIVATE external/delilah/include external/delilah/delilah)

target_link_libraries(experiment4 PRIVATE vm funcs manager)
target_link_libraries(experiment4 PRIVATE Threads::Threads)
target_include_directories(experiment4 PRIVATE external/delilah/include external/delilah/delilah)

target_link_libraries(experiment5 PRIVATE vm funcs manager)
target_link_libraries(experiment5 PRIVATE Threads::Threads)
target_include_directories(experiment5 PRIVATE external/delilah/include external/delilah/delilah)