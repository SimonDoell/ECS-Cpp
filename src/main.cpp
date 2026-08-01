#include <iostream>
#include "SparseSet.hpp"
#include "SparseView.hpp"
#include "ECS.hpp"


struct Pos {float x; float y;};
struct Vel {float x; float y;};
struct Acc {float x; float y;};
struct Name {std::string name;};
enum class AI : uint8_t {RandomWalk = 0, TargetAquired = 1};

static constexpr auto ai_to_string(AI ai) {
    if (ai == AI::RandomWalk) return "RandomWalk";
    else if (ai == AI::TargetAquired) return "TargetAquired";
    else return "Unknown";
}

template<typename... Types>
constexpr Entity create_enemy(ecs<Types...>& ecs) {
    Entity enemy = ecs.create_entity();

    ecs.template insert<Pos>(enemy, Pos{});
    ecs.template insert<Vel>(enemy, Vel{});
    ecs.template insert<Acc>(enemy, Acc{});
    ecs.template insert<Name>(enemy, Name{.name = "Enemy"});
    ecs.template insert<AI>(enemy, (AI)(rand() % 2));

    return enemy;
}



int main() {
    ecs<Pos, Vel, Acc, Name, AI> ecs;

    Entity player = ecs.create_entity();

    ecs.insert<Pos>(player, Pos{.x = 1.0f, .y = -1.0f});
    ecs.insert<Vel>(player, Vel{});
    ecs.insert<Acc>(player, Acc{});
    ecs.insert<Name>(player, Name{.name = "Player"});

    Entity enemyA = create_enemy(ecs);
    Entity enemyB = create_enemy(ecs);
    Entity enemyC = create_enemy(ecs);

    for (auto [p, v, a, n] : ecs.view<Pos, Vel, Acc, Name>(exclude<AI>))
        std::cout << "Entity: " << n.name << "\n";
    std::cout << "------------\n";
    for (auto [p, v, a, ai, n] : ecs.view<Pos, Vel, Acc, AI, Name>())
        std::cout << "Entity: " << n.name << " in mode: " << ai_to_string(ai) << "\n";

    return 0;
}