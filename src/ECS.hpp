#pragma once
#include "SparseSet.hpp"
#include "SparseView.hpp"

template<typename... Components>
struct ecs {
        using size_type = uint32_t;

        static constexpr size_type page_size = 12;
        static constexpr size_type component_count = sizeof...(Components);

        template<typename T>
        using set_type = sparse_set<T, page_size>;

    public:
        ecs() {}

        constexpr Entity create_entity() {
            if (freed_entities.empty()) return current_entity++;
            else {
                Entity entity = freed_entities.back();
                freed_entities.pop_back();
                return entity;
            }
        }

        constexpr void destroy_entity(Entity& entity) {
            freed_entities.emplace_back(entity);
            remove_all(entity);
            entity = -1;
        }

        template<typename T>
        constexpr bool insert(Entity entity, const T& value) {
            static_assert(is_component<T>());
            return get<T>().insert(entity, value);
        }

        template<typename T>
        constexpr bool remove(Entity entity) noexcept {
            static_assert(is_component<T>());
            return get<T>().remove(entity);
        }

        constexpr bool remove_all(Entity entity) noexcept {
            return (get<Components>().remove(entity), ...);
        }

        template<typename T>
        constexpr T& get(Entity entity) noexcept {
            static_assert(is_component<T>());
            return get<T>().get(entity);
        }

        template<typename T>
        constexpr const T& get(Entity entity) const noexcept {
            static_assert(is_component<T>());
            return get<T>().get(entity);
        }

        template<typename... Types>
        constexpr bool contains(Entity entity) const noexcept {
            static_assert(are_components<Types...>());
            return (get<Types>().contains(entity) && ...);
        }

        template<typename... Inc, typename... Exc>
        constexpr bool contains(Entity entity, Exclude<Exc...>) const noexcept {
            static_assert(are_components<Inc...>());
            static_assert(are_components<Exc...>());
            static_assert((not_contained_by<Inc, Exc...>() && ...));
            return (get<Inc>().contains(entity) && ...) && (!get<Exc>().contains(entity) && ...);
        }
        
        template<typename... Types, typename... Exc>
        constexpr sparse_view<TypeList<sparse_set<Types, page_size>...>, TypeList<sparse_set<Exc, page_size>...>> view(Exclude<Exc...> _exclude = exclude<>) {
            static_assert(are_components<Types...>());
            static_assert(are_components<Exc  ...>());
            if constexpr (sizeof...(Exc) >= 1) {
                return sparse_view<TypeList<sparse_set<Types, page_size>...>, TypeList<sparse_set<Exc, page_size>...>>(get<Types>()..., get<Exc...>());
            } else {
                return sparse_view<TypeList<sparse_set<Types, page_size>...>, TypeList<sparse_set<Exc, page_size>...>>(get<Types>()...);
            }
        }

        template<typename... Types, typename... Exc>
        constexpr sparse_view<TypeList<const sparse_set<Types, page_size>...>, TypeList<const sparse_set<Exc, page_size>...>> view(Exclude<Exc...> _exclude = exclude<>) const {
            static_assert(are_components<Types...>());
            static_assert(are_components<Exc  ...>());
            if constexpr (sizeof...(Exc) >= 1) {
                return sparse_view<TypeList<const sparse_set<Types, page_size>...>, TypeList<const sparse_set<Exc, page_size>...>>(get<Types>()..., get<Exc...>());
            } else {
                return sparse_view<TypeList<const sparse_set<Types, page_size>...>, TypeList<const sparse_set<Exc, page_size>...>>(get<Types>()...);
            }
        }

    private:
        std::tuple<set_type<Components>...> sets;
        std::vector<Entity> freed_entities;
        Entity current_entity = 0;

        template<typename T>
        constexpr set_type<T>& get() {
            return std::get<set_type<T>>(sets);
        }

        template<typename T>
        constexpr const set_type<T>& get() const {
            return std::get<set_type<T>>(sets);
        }

        template<typename T>
        static consteval bool is_component() {
            return (std::is_same_v<T, Components> || ...);
        }

        template<typename... Types>
        static consteval bool are_components() {
            return (is_component<Types>() && ...);
        }

        template<typename T, typename... Types>
        static constexpr bool not_contained_by() {
            return (!std::is_same_v<T, Types> && ...);
        }
};