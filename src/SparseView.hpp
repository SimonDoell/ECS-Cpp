#pragma once
#include <tuple>
#include <array>
#include <algorithm>
#include <type_traits>
#include "Config.hpp"
#include "SparseSet.hpp"

template<typename... Types>
struct TypeList {};

template<typename... Types>
inline constexpr TypeList<Types...> exclude{};



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
struct sparse_view<TypeList<Inc...>, TypeList<Exc...>> {
        using size_type = entity_traits_v::size_type;

        template<typename Tp>
        using value_type = std::conditional_t<std::is_const_v<Tp>, const typename Tp::value_type, typename Tp::value_type>;
        
        using reference = std::tuple<value_type<Inc>&...>;

        static constexpr size_type include_count = sizeof...(Inc);
        static constexpr size_type exclude_count = sizeof...(Exc);

        static_assert(include_count >= 1);
        static_assert((is_unique_v<std::remove_cvref_t<Inc>..., std::remove_cvref_t<Exc>...>));
        static_assert((std::is_same_v<std::remove_cvref_t<Inc>, sparse_set<typename Inc::value_type>> && ...));
        static_assert((std::is_same_v<std::remove_cvref_t<Exc>, sparse_set<typename Exc::value_type>> && ...));

        struct iterator {
            public:
                iterator(const sparse_view& _view, size_type _packed_index)
                : view(_view), packed_index(_view.find_next(_packed_index)) {}

                constexpr bool operator!=(const iterator& other) const {
                    return packed_index != other.packed_index;
                }

                constexpr reference operator*() const {
                    return view.values_from_packed(packed_index);
                }

                constexpr iterator& operator++() {
                    packed_index = view.find_next(packed_index + 1);
                    return *this;
                }

            private:
                const sparse_view& view;
                size_type packed_index;
        };
    
    public:
        sparse_view(Inc&... _include_list, Exc&... _exclude_list)
        : sets{&_include_list..., &_exclude_list...}, leading_size(static_cast<size_type>(-1)) {
            (leading_helper(_include_list), ...);
        }

        constexpr iterator begin() const {return iterator(*this, 0);}
        constexpr iterator end()   const {return iterator(*this, leading_size);}

    private:
        std::tuple<Inc*..., Exc*...> sets;
        const std::vector<Entity>* leading_packed;
        const void* leading_storage;
        size_type leading_size;

        
        constexpr void leading_helper(const auto& set) noexcept {
            if (leading_size < set.size() || leading_size == static_cast<size_type>(-1)) {
                leading_packed  = set.packed_ptr();
                leading_storage = &set;
                leading_size    = set.size();
            }
        }

        template<typename SparseSetType>
        constexpr bool contains(Entity entity) const noexcept {
            auto set = std::get<SparseSetType*>(sets);
            return (set == leading_storage) || (set->contains(entity));
        }

        constexpr bool is_valid(Entity entity) const noexcept {
            return
                ( contains<Inc>(entity) && ...) &&
                (!contains<Exc>(entity) && ...);
        }

        constexpr size_type find_next(size_type packed_index) const noexcept {
            while (packed_index < leading_size) {
                Entity entity = (*leading_packed)[packed_index];
                
                if (is_valid(entity))
                    return packed_index;
                else packed_index++;
            }

            return leading_size;
        }

        constexpr reference values_from_packed(size_type packed_index) const noexcept {
            Entity entity = (*leading_packed)[packed_index];
            return {(*(std::get<Inc*>(sets)->try_get(entity)))...};
        }
};