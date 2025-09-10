#include "hps/hps.hpp"

#include <iostream>

int main() {
    auto version = hps::version();
    std::cout << "HPS version: " << version << std::endl;
    system("pause");
}