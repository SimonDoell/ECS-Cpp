#pragma once
#include <tuple>
#include <array>
#include <algorithm>
#include "SparseSet.hpp"

template<typename... Rest>
struct is_unique;

template<>
struct is_unique<> {
    static constexpr bool value = true;
};

template<typename T, typename... Rest>
struct is_unique<T, Rest...> {
  static constexpr bool value =
      ((!std::is_same_v<T, Rest> && ...) && (is_unique<Rest...>::value));
};

template<typename... Types>
inline constexpr bool is_unique_v = is_unique<Types...>::value;


template<typename T, typename... TypePack>
consteval uint32_t index_of() {
    static_assert((is_unique_v<TypePack...>));
    static_assert((std::is_same_v<T, TypePack> || ...));
    constexpr bool matches[sizeof...(TypePack)] = {std::is_same_v<T, TypePack>...};
    for (uint32_t i = 0; i < sizeof...(TypePack); ++i) if (matches[i] == true) return i;
    return ~0u;
}


template<typename... ContainedTypes>
struct sparse_view {
        using size_type = uint32_t;
        using reference = std::tuple<ContainedTypes&...>;
        static constexpr size_type type_count = static_cast<size_type>(sizeof...(ContainedTypes));

        using pageType   = std::unique_ptr<size_type[]>;
        using sparseType = std::vector<pageType>;
        using denseType  = std::vector<Entity>;

        static_assert(type_count >= 2);
        static_assert(is_unique_v<ContainedTypes...>);

    private:
        struct type_entry {
            size_type type_index;
            size_type dense_size;
            void* instance;
            bool(*contains)(void*, Entity) noexcept;
        };

        template<typename T>
        static constexpr bool contains_wrapper(void* ptr, Entity entity) noexcept {
            return static_cast<sparse_set<T>*>(ptr)->contains(entity);
        }

    public:
        struct iterator {
            public:
                iterator(sparse_view* _view, size_type _dense_index)
                : view(_view), dense_index(_dense_index) {}

                constexpr bool operator!=(const iterator& other) {
                    return dense_index != other.dense_index;
                }

                constexpr reference operator*() {
                    return view->values_from_dense(dense_index);
                }

                constexpr iterator& operator++() {
                    dense_index = view->find_next_dense(dense_index + 1);
                    return *this;
                }

            private:
                sparse_view* view;
                size_type dense_index;
        };

    public:
        sparse_view(sparse_set<ContainedTypes>&... _sparse_sets)
        : sparse_sets{_sparse_sets...} {
            size_type index = 0;
            ((entrys[index++] = type_entry{
                .type_index = index_of<ContainedTypes, ContainedTypes...>(),
                .dense_size = static_cast<size_type>(_sparse_sets.size()),
                .instance   = &_sparse_sets,
                .contains   = &contains_wrapper<ContainedTypes>
            }), ...);

            std::sort(entrys.begin(), entrys.end(), [](const type_entry& a, const type_entry& b){
                return a.dense_size < b.dense_size;
            });

            smallest_dense_size = entrys[0].dense_size;
            highest_dense_size  = entrys.back().dense_size;

            (first_array_helper(_sparse_sets), ...);
        }

        constexpr iterator begin() {return iterator(this, find_next_dense(0));}
        constexpr iterator end()   {return iterator(this, smallest_dense_size);}

    private:
        std::tuple<sparse_set<ContainedTypes>&...> sparse_sets;
        std::array<type_entry, type_count> entrys;
        size_type smallest_dense_size  = 0;
        size_type highest_dense_size   = 0;
        denseType  const* first_dense  = nullptr;

        template<typename T>
        constexpr void first_array_helper(sparse_set<T>& _sparse_sets) {
            if (entrys[0].type_index == index_of<T, ContainedTypes...>()) {
                first_dense = _sparse_sets.dense_ptr();
            }
        }

        constexpr bool contained_by_all(size_type dense_index) {
            if (dense_index >= first_dense->size()) return false;
            Entity entity = (*first_dense)[dense_index];

            for (size_type i = 1; i < type_count; ++i)
                if (!entrys[i].contains(entrys[i].instance, entity)) return false;

            return true;
        }

        constexpr size_type find_next_dense(size_type dense_index) {
            while (dense_index < smallest_dense_size) {
                if (contained_by_all(dense_index)) return dense_index;
                else dense_index++;
            }

            return smallest_dense_size;
        }

        constexpr reference values_from_dense(size_type dense_index) {
            assert(dense_index < first_dense->size());
            Entity entity = (*first_dense)[dense_index];
            
            return {
                std::get<sparse_set<ContainedTypes>&>(sparse_sets).get(entity)...
            };
        }
};