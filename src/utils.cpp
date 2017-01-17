#include "utils.h"

void print_indent(size_t indent) {
    for (size_t i = 0; i < indent; i++) {
        std::cout << "  ";
    }
}
