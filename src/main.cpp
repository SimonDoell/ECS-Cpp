#include <iostream>
#include "SparseSet.hpp"
#include "SparseView.hpp"
#include "ECS.hpp"


struct Pos {float x; float y;};
struct Vel {float x; float y;};
struct Acc {float x; float y;};
struct Health {float health;};
enum class Weapon {Sword, Pistol, None};
enum class AI {Wander, Attack};

using ECS = ecs<Pos, Vel, Acc, Health, Weapon, AI>;


constexpr Entity create_enemy(ECS& ecs) {
    Entity enemy = ecs.create_entity();
    ecs.insert(enemy, Pos{});
    ecs.insert(enemy, Vel{});
    ecs.insert(enemy, Acc{});
    ecs.insert(enemy, Health{});
    ecs.insert(enemy, Weapon::None);
    ecs.insert(enemy, AI::Wander);
    return enemy;
}



int main() {
    ECS ecs;

    Entity player = ecs.create_entity();
    ecs.insert(player, Pos{});
    ecs.insert(player, Vel{});
    ecs.insert(player, Acc{});

    Entity enemyA = create_enemy(ecs);
    Entity enemyB = create_enemy(ecs);

    for (auto [p, v, a] : ecs.view<Pos, Vel, Acc>(exclude<AI>)) {
        std::cout << p.x << "\n";
        std::cout << v.x << "\n";
        std::cout << a.x << "\n";
        std::cout << "\n";
    }

    std::cout << ecs.get<Pos>(player).x << "\n";
    
    return 0;
}