#include <cstdio>
#include <cstring>
#include <fstream>
#include "logs.h"
#include "iommap.h"
#include "ast/tree.h"
#include "raii/memory_map.h"
#include "raii/ast_tree.h"
#include "backend/llvm/ir_gen.h"

int
main( int argc,
      char *argv[])
{
    if (argc != 3) {
            fprintf(stderr, ascii(RED, "There must be 2 arguments: %s [input.tree] [output.ll]\n"), argv[0]);
            return EXIT_FAILURE;
    }

    const char *src_file = argv[1];
    const char *out_file = argv[2];

    try {

        MemoryMap map{ src_file};
        Tree ast_tree{ map.data.buf};
        map.unmap();

        dump_tree( ast_tree.root());

        IRGenerator irgen{};
        irgen.compile( ast_tree.root());

        std::fstream out{ out_file, out.trunc | out.out};
        out << "Hello world" << 2 << '\n' << std::endl;

    } catch ( const std::exception& exception )
    {
        std::fprintf(stderr, ascii(RED, "Compilation failed: %s\n"), exception.what());
    }
}


