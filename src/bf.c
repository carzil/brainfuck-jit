#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include "bf.h"
#include "vm.h"
#include "compiler.h"
#include <time.h>

void default_options() {
    bf_options.debug = false;
    bf_options.dump_jit = false;
    bf_options.pretty_print = false;
    bf_options.optimize_level = 2;
}

int main(int argc, char **argv) {
    default_options();

    static struct option long_options[] = {
        {"dump-jit", no_argument, 0, 'j'},
        {"debug", no_argument, 0, 'd'},
        {"pretty-print", no_argument, 0, 'p'},
        {"optimize", required_argument, 0, 'O'},
        {0, 0, 0, 0},
    };

    int option_index;
    int c;
    while ((c = getopt_long(argc, argv, "djpO:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'd':
                bf_options.debug = true;
                break;
            case 'j':
                bf_options.dump_jit = true;
                break;
            case 'p':
                bf_options.pretty_print = true;
                break;
            case 'O':
                bf_options.optimize_level = atoi(optarg);
                break;
            case '?':
            default:
                return 1;
        }
    }
    if (optind >= argc) {
        printf("bf: no file specified!\n");
    } else {
        clock_t start = clock();
        bf_vm* vm = bf_compile_file(argv[optind]);
        double elapsed_time = 1.0 * (clock() - start) / CLOCKS_PER_SEC;
        // printf("compilation took %.6f secs\n", elapsed_time);

        start = clock();
        bf_vm_run(vm);
        elapsed_time = 1.0 * (clock() - start) / CLOCKS_PER_SEC;
        // printf("run took %.6f secs\n", elapsed_time);

        bf_vm_free(vm);
    }
    return 0;
}
