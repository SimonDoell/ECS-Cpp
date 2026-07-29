#pragma once
#include <memory>
#include <vector>
#include <cstdint>
#include <cassert>
#include <type_traits>

using Entity = uint32_t;

template<typename T, uint32_t PageSizePower = 12>
struct sparse_set {
        using value_type = T;
        using size_type  = uint32_t;
        using reference  = T&;
        using pointer    = T*;
        using const_reference  = const T&;
        using const_pointer    = const T*;
        
        static constexpr size_type page_size_power = PageSizePower;
        static constexpr size_type page_size       = 1 << page_size_power;
        static constexpr size_type page_shift      = page_size_power;
        static constexpr size_type page_mask       = page_size - 1;
        static constexpr size_type tomb_value      = ~static_cast<size_type>(0);

        using pageType   = std::unique_ptr<size_type[]>;
        using sparseType = std::vector<pageType>;
        using denseType  = std::vector<Entity>;
        using valueType  = std::vector<value_type>;

        static_assert(page_size_power >= 2);
        static_assert(page_size_power <= sizeof(size_type) * 8 - 1);

    private:
        struct position {
            size_type page_index;
            size_type local_index;
        };
    
    public:
        sparse_set()                                   = default;
        sparse_set(sparse_set&& other)                 = default;
        sparse_set& operator=(sparse_set&& other)      = default;
        sparse_set(const sparse_set& other)            = delete;
        sparse_set& operator=(const sparse_set& other) = delete;

        constexpr bool insert(Entity entity, const_reference value) {
            auto [page_index, local_index] = get_position(entity);

            size_type dense_index = try_contains(page_index, local_index);
            if (dense_index != tomb_value) return false;
            ensure_page(page_index);

            sparse[page_index][local_index] = dense.size();
            dense.push_back(entity);
            values.push_back(value);

            return true;
        }

        constexpr bool contains(Entity entity) const noexcept {
            auto [page_index, local_index] = get_position(entity);
            
            if (page_index >= sparse.size())                   return false;
            if (sparse[page_index] == nullptr)                 return false;
            if (sparse[page_index][local_index] == tomb_value) return false;
            return true;
        }

        constexpr bool remove(Entity entity) noexcept {
            auto [page_index, local_index] = get_position(entity);

            size_type dense_index = try_contains(page_index, local_index);
            if (dense_index == tomb_value) return false;

            size_type last_dense_index = dense.size() - 1;
            auto [last_page_index, last_local_index] = get_position(dense[last_dense_index]);

            if (dense_index != last_dense_index) {
                dense [dense_index] = dense[last_dense_index];
                values[dense_index] = std::move(values.back());
                sparse[last_page_index][last_local_index] = dense_index;
            }

            sparse[page_index][local_index] = tomb_value;
            dense .pop_back();
            values.pop_back();

            return true;
        }

        constexpr reference get(Entity entity) noexcept {
            auto [page_index, local_index] = get_position(entity);
            size_type dense_index = try_contains(page_index, local_index);
            assert(dense_index != tomb_value);
            return values[dense_index];
        }

        constexpr const_reference get(Entity entity) const noexcept {
            auto [page_index, local_index] = get_position(entity);
            size_type dense_index = try_contains(page_index, local_index);
            assert(dense_index != tomb_value);
            return values[dense_index];
        }

        auto begin()       noexcept {return values.begin();}
        auto begin() const noexcept {return values.begin();}
        auto end()         noexcept {return values.end();}
        auto end()   const noexcept {return values.end();}
        
    private:
        sparseType sparse;
        denseType  dense;
        valueType  values;
        
        static constexpr position get_position(Entity entity) noexcept {
            return position{
                .page_index  = (entity >> page_shift),
                .local_index = (entity  & page_mask )
            };
        }

        constexpr void ensure_page(size_type page_index) {
            if (page_index >= sparse.size()) {
                sparse.resize(page_index + 1);
            }

            if (sparse[page_index] == nullptr) {
                sparse[page_index] = std::make_unique<size_type[]>(page_size);
                std::fill_n(sparse[page_index].get(), page_size, tomb_value);
            }
        }

        constexpr bool contains(size_type page_index, size_type local_index) const noexcept {
            if (page_index >= sparse.size())                   return false;
            if (sparse[page_index] == nullptr)                 return false;
            if (sparse[page_index][local_index] == tomb_value) return false;
            return true;
        }

        constexpr size_type try_contains(size_type page_index, size_type local_index) const noexcept {
            if (page_index >= sparse.size())   return tomb_value;
            if (sparse[page_index] == nullptr) return tomb_value;
            return sparse[page_index][local_index];
        }
};