#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include "Config.hpp"

template<typename Tp>
struct sparse_set {
        using size_type  = entity_traits_v::size_type;
        using value_type = Tp;
        using reference  = Tp&;
        using pointer    = Tp*;
        using const_reference  = const Tp&;
        using const_pointer    = const Tp*;

        static constexpr size_type page_size_power = component_traits<Tp>::page_size_power;
        static constexpr size_type page_size       = 1 << page_size_power;
        static constexpr size_type page_shift      = page_size_power;
        static constexpr size_type page_mask       = page_size - 1;
        static constexpr size_type tomb_value      = ~static_cast<size_type>(0);

        using page_type   = std::unique_ptr<size_type[]>;
        using sparse_type = std::vector<page_type>;
        using packed_type = std::vector<Entity>;
        using values_type = std::vector<value_type>;

        static_assert(page_size_power >= 2);
        static_assert(page_size_power <= 16);
        static_assert(entity_traits_v::index_mask == ~entity_traits_v::generation_mask);

    private:
        struct position {
            size_type paged_index;
            size_type local_index;
        };
    
    public:
        sparse_set()                                   = default;
        sparse_set(sparse_set&& other)                 = default;
        sparse_set& operator=(sparse_set&& other)      = default;
        sparse_set(const sparse_set& other)            = delete;
        sparse_set& operator=(const sparse_set& other) = delete;

        constexpr bool insert(Entity entity, const_reference value) {
            auto [paged_index, local_index] = get_position(entity);
            size_type packed_index = try_contain(paged_index, local_index);

            if (packed_index != tomb_value) return false;
            ensure_page(paged_index);

            packed_index = packed.size();
            sparse[paged_index][local_index] = packed_index;
            packed.push_back(entity);
            values.push_back(value);

            return true;
        }

        constexpr bool remove(Entity entity) {
            auto [paged_index, local_index] = get_position(entity);
            size_type packed_index = try_contain(paged_index, local_index);

            if (packed_index == tomb_value) return false;

            size_type last_packed_index = packed.size() - 1;
            auto [last_paged_index, last_local_index] = get_position(packed[last_packed_index]);

            if (packed_index != last_packed_index) {
                packed[packed_index] = packed[last_packed_index];
                values[packed_index] = std::move(values[last_packed_index]);
                sparse[last_paged_index][last_local_index] = packed_index;
            }

            sparse[paged_index][local_index] = tomb_value;
            packed.pop_back();
            values.pop_back();

            return true;
        }

        constexpr bool contains(Entity entity) const {
            auto [paged_index, local_index] = get_position(entity);
            size_type packed_index = try_contain(paged_index, local_index);
            if (packed_index == tomb_value) return false;
            return (get_generation(entity) == get_generation(packed[packed_index]));
        }
        
        constexpr pointer try_get(Entity entity) {
            auto [paged_index, local_index] = get_position(entity);
            size_type packed_index = try_contain(paged_index, local_index);
            if (packed_index == tomb_value) return nullptr;
            if (get_generation(entity) != get_generation(packed[packed_index])) return nullptr;
            return &values[packed_index];
        }

        constexpr const_pointer try_get(Entity entity) const {
            auto [paged_index, local_index] = get_position(entity);
            size_type packed_index = try_contain(paged_index, local_index);
            if (packed_index == tomb_value) return nullptr;
            if (get_generation(entity) != get_generation(packed[packed_index])) return nullptr;
            return &values[packed_index];
        }

        constexpr const sparse_type* sparse_ptr() const {return &sparse;}
        constexpr const packed_type* packed_ptr() const {return &packed;}

        constexpr auto begin() {return values.begin();}
        constexpr auto end()   {return values.end();}
        constexpr auto begin() const {return values.begin();}
        constexpr auto end()   const {return values.end();}

        constexpr bool empty() const noexcept {return packed.empty();}
        constexpr auto size()  const noexcept {return packed.size();}

        constexpr void clear() {
            sparse.clear();
            packed.clear();
            values.clear();
        }

    private:
        sparse_type sparse;
        packed_type packed;
        values_type values;

        static constexpr position get_position(Entity entity) noexcept {
            size_type entity_index = entity_traits_v::index_of(entity);
            return position{
                .paged_index = entity_index >> page_shift,
                .local_index = entity_index  & page_mask,
            };
        }

        static constexpr size_type get_generation(Entity entity) noexcept {
            return entity_traits_v::generation_of(entity);
        }

        constexpr void ensure_page(size_type paged_index) {
            if (paged_index >= sparse.size()) {
                sparse.resize(paged_index + 1);
            }

            if (sparse[paged_index] == nullptr) {
                sparse[paged_index] = std::make_unique<size_type[]>(page_size);
                std::fill_n(sparse[paged_index].get(), page_size, tomb_value);
            }
        }
        
        constexpr size_type try_contain(size_type paged_index, size_type local_index) const {
            if (paged_index >= sparse.size())   return tomb_value;
            if (sparse[paged_index] == nullptr) return tomb_value;
            return sparse[paged_index][local_index];
        }
};