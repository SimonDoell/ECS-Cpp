#pragma once
#include <tuple>
#include <array>
#include <algorithm>
#include "SparseSet.hpp"

template<typename T, typename... Types>
consteval uint32_t index_of() {
    static_assert((std::is_same_v<T, Types> || ...));
    static_assert((sizeof...(Types) >= 1));
    bool matches[sizeof...(Types)] = {std::is_same_v<T, Types>...};
    for (uint32_t i = 0; i < sizeof...(Types); ++i) if (matches[i]) return i;
    return -1;
}

template<typename... SparseSets>
struct sparse_view {
        using size_type = uint32_t;
        using reference = std::tuple<typename SparseSets::value_type...>;

        static constexpr size_type type_count = sizeof...(SparseSets);

        static_assert((type_count >= 2));
        static_assert((std::is_same_v<SparseSets, sparse_set<typename SparseSets::value_type, SparseSets::page_size_power>> && ...));

    public:
        struct iterator {
            public:
                iterator(sparse_view* _view, size_type _dense_index)
                : view(_view), dense_index(view->find_next(_dense_index)) {
                    assert(_view != nullptr);
                }

                constexpr iterator& operator++() {
                    dense_index = view->find_next(dense_index + 1);
                    return *this;
                }

                constexpr bool operator !=(const iterator& other) {
                    return dense_index != other.dense_index;
                }

                constexpr reference operator*() {
                    return view->value_from_dense(dense_index);
                }

            private:
                sparse_view* view;
                size_type dense_index;
        };

    public:
        sparse_view(SparseSets&... _sparse_sets)
        : sparse_sets{&_sparse_sets...} {
            (smallest_helper(_sparse_sets), ...);
        }

        constexpr iterator begin() {return iterator(this, 0);}
        constexpr iterator end()   {return iterator(this, smallest_size);}

    private:
        std::tuple<SparseSets*...> sparse_sets;
        std::vector<size_type> const* leading_dense;
        size_type leading_type_index;
        size_type smallest_size = -1;

        template<typename T, uint32_t PageSizePower = 12>
        constexpr void smallest_helper(sparse_set<T, PageSizePower>& set) {
            if (smallest_size == static_cast<size_type>(-1) || smallest_size > set.size()) {
                leading_type_index = index_of<sparse_set<T, PageSizePower>, SparseSets...>();
                leading_dense      = set.dense_ptr();
                smallest_size      = set.size();
            }
        }

        template<typename T, uint32_t PageSizePower = 12>
        constexpr bool contained_helper(sparse_set<T, PageSizePower>& set, Entity entity) const {
            if (leading_type_index == index_of<sparse_set<T, PageSizePower>, SparseSets...>()) {
                return true;
            } else {
                return set.contains(entity);
            }
        }

        constexpr bool contained_by_all(Entity entity) const {
            return (contained_helper(*std::get<SparseSets*>(sparse_sets), entity) && ...);
        }

        constexpr size_type find_next(size_type dense_index) const {
            while (dense_index < smallest_size) {
                Entity entity = (*leading_dense)[dense_index];
                
                if (contained_by_all(entity)) return dense_index;
                else dense_index++;
            }

            return smallest_size;
        }

        constexpr reference value_from_dense(size_type dense_index) {
            assert(dense_index < leading_dense->size() && "value_from_dense");
            Entity entity = (*leading_dense)[dense_index];

            return {(std::get<SparseSets*>(sparse_sets)->get(entity))...};
        }
};