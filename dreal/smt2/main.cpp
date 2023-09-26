/**
 * @file Temp.cpp
 * @author dlinear
 * @date 22 Aug 2023
 * @copyright 2023 dlinear
 */

#include "dreal/smt2/run.h"
#include "dreal/util/infty.h"
#include "dreal/qsopt_ex.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <smt2 file>" << std::endl;
        return 1;
    }
    dreal::util::InftyStart(1e6);
    dreal::qsopt_ex::QSXStart();

    dreal::Config config{};
    dreal::RunSmt2(argv[1], config, true, true);

    dreal::qsopt_ex::QSXFinish();
    dreal::util::InftyFinish();
    return 0;
}