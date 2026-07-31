#pragma once
#include <cstdint>

using Entity = uint32_t;

template<typename EntityType>
struct entity_traits;

using entity_traits_v = entity_traits<Entity>;

template<>
struct entity_traits<uint32_t> {
    using size_type   = uint32_t;
    using entity_type = uint32_t;
    static constexpr entity_type index_mask      = 0b00000000'00111111'11111111'11111111;
    static constexpr entity_type index_bits      = 22;
    static constexpr entity_type generation_mask = 0b11111111'11000000'00000000'00000000;
    static_assert(index_mask == ~generation_mask);

    static constexpr size_type index_of(entity_type entity) {
        return entity & index_mask;
    }

    static constexpr size_type generation_of(entity_type entity) {
        return ((entity & generation_mask) >> index_bits);
    }

    static constexpr entity_type set_index(entity_type entity, size_type index) {
        return ((entity & generation_mask) | (index & index_mask));
    }

    static constexpr entity_type set_generation(entity_type entity, size_type generation) {
        return (entity & index_mask) | ((static_cast<entity_type>(generation) << index_bits) & generation_mask);
    }
};

template<>
struct entity_traits<uint64_t> {
    using size_type   = uint32_t;
    using entity_type = uint64_t;
    static constexpr uint64_t index_mask      = 0b00000000'00000000'00000000'00000000'11111111'11111111'11111111'11111111;
    static constexpr uint64_t index_bits      = 32;
    static constexpr uint64_t generation_mask = 0b11111111'11111111'11111111'11111111'00000000'00000000'00000000'00000000;
    static_assert(index_mask == ~generation_mask);

    static constexpr size_type index_of(entity_type entity) {
        return entity & index_mask;
    }

    static constexpr size_type generation_of(entity_type entity) {
        return ((entity & generation_mask) >> index_bits);
    }

    static constexpr entity_type set_index(entity_type entity, size_type index) {
        return ((entity & generation_mask) | (index & index_mask));
    }

    static constexpr entity_type set_generation(entity_type entity, size_type generation) {
        return (entity & index_mask) | ((static_cast<entity_type>(generation) << index_bits) & generation_mask);
    }
};






template<typename Tp>
struct component_traits {
    static constexpr uint32_t page_size_power = 12;
};

template<>
struct component_traits<Entity> {
    static constexpr uint32_t page_size_power = 0;
};