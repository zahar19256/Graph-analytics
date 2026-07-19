#include <stdexcept>
#include "Executor.h"

// TODO добавить потом параметр запуска на вариант сходимость или предел по итерациям

int main(int argc , char* argv[]) {
    if (argc != 3) {
        throw std::runtime_error("Wrong arguments count expected 2, but get: " + std::to_string(argc));
    }
    Executor engine(argv[1] , argv[2]);
    engine.Compute();
}
