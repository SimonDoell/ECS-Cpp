#include <iostream>
#include "SparseSet.hpp"
#include "SparseView.hpp"
#include "ECS.hpp"

int main() {
    sparse_set<int>   a;
    sparse_set<char>  b;
    sparse_set<float> c;

    a.insert(0, 1);
    b.insert(0, 'A');
    c.insert(0, 1.0f);

    a.insert(1, 2);
    b.insert(1, 'B');
    c.insert(1, 2.0f);

    c.insert(2, 2.0f);
    c.insert(3, 2.0f);

    for (auto [aa, bb, cc] : sparse_view(a, b, c)) {
        std::cout << aa << "\n";
        std::cout << bb << "\n";
        std::cout << cc << "\n";
        std::cout << "\n";
    }
    
    return 0;
}