#include <stdio.h>
#include "vm.h"
#include "compiler.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("specify file to run\n");
        return 1;
    }
    bf_vm* vm = bf_compile_file(argv[1]);
    vm_run(vm);
    vm_free(vm);
    return 0;
}