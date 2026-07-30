#include <iostream>
#include "SparseSet.hpp"
#include "SparseView.hpp"
#include "ECS.hpp"

int main() {
    ecs<int, char, float> ecs;

    for (auto [i, c, f] : ecs.view<int, char, float>()) {
        std::cout << i << "\n";
        std::cout << c << "\n";
        std::cout << f << "\n";
    }
    
    return 0;
}