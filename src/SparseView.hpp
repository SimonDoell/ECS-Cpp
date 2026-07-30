#pragma once
#include <tuple>
#include <array>
#include <algorithm>
#include "SparseSet.hpp"


template<typename... SparseSets>
struct sparse_view {
        using size_type = uint32_t;
        using reference = std::tuple<std::conditional_t<
            std::is_const_v<SparseSets>, 
            const typename SparseSets::value_type, 
            typename SparseSets::value_type
        >&...>;

        static constexpr size_type type_count = sizeof...(SparseSets);

        static_assert((type_count >= 1));
        static_assert((
            std::is_same_v<std::remove_const_t<SparseSets>,
            std::remove_const_t<sparse_set<typename SparseSets::value_type, SparseSets::page_size_power>>> && ...)
        );

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

                constexpr bool operator!=(const iterator& other) {
                    return dense_index != other.dense_index;
                }

                constexpr reference operator*() {
                    return view->value_from_dense(dense_index);
                }

            private:
                sparse_view* view;
                size_type dense_index;
        };

        struct const_iterator {
            public:
                const_iterator(const sparse_view* _view, size_type _dense_index)
                : view(_view), dense_index(view->find_next(_dense_index)) {
                    assert(_view != nullptr);
                }

                constexpr const_iterator& operator++() {
                    dense_index = view->find_next(dense_index + 1);
                    return *this;
                }

                constexpr bool operator!=(const const_iterator& other) {
                    return dense_index != other.dense_index;
                }

                constexpr reference operator*() const {
                    return view->value_from_dense(dense_index);
                }

            private:
                const sparse_view* view;
                size_type dense_index;
        };

    public:
        sparse_view(SparseSets&... _sparse_sets)
        : sparse_sets{&_sparse_sets...}, smallest_size(-1) {
            (smallest_helper(_sparse_sets), ...);
        }

        constexpr iterator begin() {return iterator(this, 0);}
        constexpr iterator end()   {return iterator(this, smallest_size);}

        constexpr const_iterator begin() const {return const_iterator(this, 0);}
        constexpr const_iterator end()   const {return const_iterator(this, smallest_size);}

    private:
        std::tuple<SparseSets*...> sparse_sets;
        std::vector<Entity> const* leading_dense;
        void const* leading_storage;
        size_type smallest_size;

        template<typename T, uint32_t PageSizePower = 12>
        constexpr void smallest_helper(const sparse_set<T, PageSizePower>& set) {
            if (smallest_size == static_cast<size_type>(-1) || smallest_size > set.size()) {
                leading_dense      = set.dense_ptr();
                smallest_size      = set.size();
                leading_storage    = &set;
            }
        }

        template<typename T, uint32_t PageSizePower = 12>
        constexpr bool contained_helper(const sparse_set<T, PageSizePower>& set, Entity entity) const {
            return (leading_storage == &set) || (set.contains(entity));
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
            assert(dense_index < leading_dense->size());
            Entity entity = (*leading_dense)[dense_index];
            return {(std::get<SparseSets*>(sparse_sets)->get(entity))...};
        }

        constexpr reference value_from_dense(size_type dense_index) const {
            assert(dense_index < leading_dense->size());
            Entity entity = (*leading_dense)[dense_index];
            return {(std::get<SparseSets*>(sparse_sets)->get(entity))...};
        }
};


// ToDo:
// ### 3. Non-Standard Iterator Implementation
// • Problem: The iterator and const_iterator structs:
//     • Lack standard member types required by C++ iterator traits (like iterator_category, difference_type, value_type, pointer, and reference).
//     • Lack the post-increment operator (operator++(int)).
//     • Lack the equality operator (operator==).
// • Impact: While it works inside a basic range-based for loop, it will fail to compile if used with standard library algorithms (like std::for_each or std::find_if) or C++20
// ranges/concepts.