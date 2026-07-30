#pragma once
#include <tuple>
#include <array>
#include <algorithm>
#include "SparseSet.hpp"

template<typename... Types>
struct Exclude {};

template<typename... Types>
inline constexpr Exclude<Types...> exclude{};


template<typename... Types>
struct TypeList {};

template<typename... Types>
inline constexpr TypeList<Types...> typeList{};

template<typename T, typename... Types>
struct is_unique;

template<typename T>
struct is_unique<T> {static constexpr bool value = true;};

template<typename T, typename... Types>
struct is_unique {
    static constexpr bool value = (!std::is_same_v<T, Types> && ...) && (is_unique<Types...>::value);
};

template<typename T, typename... Types>
inline constexpr bool is_unique_v = is_unique<T, Types...>::value;



template<typename IncludeList, typename ExcludeList>
struct sparse_view;

template<typename... Inc, typename... Exc>
requires (sizeof...(Inc) >= 1)
struct sparse_view<TypeList<Inc...>, TypeList<Exc...>> {
        using size_type = uint32_t;
        static constexpr size_type include_count = sizeof...(Inc);
        static constexpr size_type exclude_count = sizeof...(Exc);

        using reference = std::tuple<
            std::conditional_t<std::is_const_v<Inc>,
            const typename Inc::value_type,
                  typename Inc::value_type
        >&...>;

        static_assert((std::is_same_v<std::remove_const_t<Inc>, sparse_set<typename Inc::value_type, Inc::page_size_power>> && ...));
        static_assert((std::is_same_v<std::remove_const_t<Exc>, sparse_set<typename Exc::value_type, Exc::page_size_power>> && ...));
        static_assert((is_unique_v<Inc..., Exc...>));

        struct iterator {
            public:
                iterator(const sparse_view* _view, size_type _dense_index)
                : view(_view), dense_index(_view->find_next(_dense_index)) {}

                constexpr bool operator!=(const iterator& other) const {
                    return dense_index != other.dense_index;
                }

                constexpr iterator& operator++() {
                    dense_index = view->find_next(dense_index + 1);
                    return *this;
                }

                constexpr reference operator*() {
                    return view->values_from_dense(dense_index);
                }

            private:
                const sparse_view* view;
                size_type dense_index;
        };
        
    public:
        sparse_view(Inc&... _include_sets, Exc&... _exclude_sets)
        : sparse_sets{&_include_sets..., &_exclude_sets...}, smallest_size(static_cast<size_type>(-1)) {
            (smallest_set(_include_sets), ...);
        }

        constexpr iterator begin() const {return iterator(this, 0);}
        constexpr iterator end()   const {return iterator(this, smallest_size);}

        
    private:
        mutable std::tuple<Inc*..., Exc*...> sparse_sets;
        const std::vector<Entity>* leading_dense;
        void const* leading_storage;
        size_type smallest_size;

        constexpr void smallest_set(const auto& set) {
            if (set.size() < smallest_size || smallest_size == static_cast<size_type>(-1)) {
                leading_storage = &set;
                leading_dense   = set.dense_ptr();
                smallest_size   = set.size();
            }
        }

        template<typename SetType>
        constexpr bool contains(Entity entity) const noexcept {
            auto set = std::get<SetType*>(sparse_sets);
            return (set == leading_storage || set->contains(entity));
        }

        constexpr bool is_valid(Entity entity) const noexcept {
            return
                ( contains<Inc>(entity) && ...) &&
                (!contains<Exc>(entity) && ...);
        }

        constexpr size_type find_next(size_type dense_index) const noexcept {
            while (dense_index < smallest_size) {
                Entity entity = (*leading_dense)[dense_index];
                if (is_valid(entity)) return dense_index;
                else dense_index++;
            }
            return smallest_size;
        }

        constexpr reference values_from_dense(size_type dense_index) const noexcept {
            Entity entity = (*leading_dense)[dense_index];
            return {std::get<Inc*>(sparse_sets)->get(entity)...};
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