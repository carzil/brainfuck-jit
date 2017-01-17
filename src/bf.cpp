#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "bf.h"
#include "vm.h"
#include "compiler.h"
#include "optimizer.h"

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
        BfCompiler compiler;
        BfOptimizer optimizer;
        BfVM vm;

        clock_t begin = clock();

        BfProgram program = compiler.CompileFile(argv[optind]);
        optimizer.Optimize(program, bf_options.optimize_level);
        program.Print();
        vm.CompileProgram(program);
        
        clock_t end = clock();

        double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        std::cout << "compilation done in " << elapsed_secs << " secs" << std::endl;

        vm.Run();
    }
    return 0;
}
