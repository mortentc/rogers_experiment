cmake_minimum_required(VERSION 3.12.2)
project(rogers_experiment VERSION 0.1.0 LANGUAGES C)

add_executable(rogers_experiment main.c)

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
target_include_directories(rogers_experiment PRIVATE external/delilah/include external/delilah/delilah)