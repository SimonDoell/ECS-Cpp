#pragma once
#include "Config.hpp"
#include "SparseSet.hpp"
#include "SparseView.hpp"


template<typename... Components>
struct ecs {
        using size_type = entity_traits_v::size_type;

        template<typename Tp>
        using set_type = sparse_set<std::remove_cvref_t<Tp>>;

        static constexpr size_type component_count = sizeof...(Components);

        static_assert(is_unique_v<std::remove_cvref_t<Components>...>);
    
    public:
        ecs() {}

        constexpr Entity create_entity() {
            if (freed_list.empty()) {
                Entity entity = 0;
                entity = entity_traits_v::set_generation(entity, 0);
                entity = entity_traits_v::set_index(entity, entity_index);
                entity_index++;
                return entity;
            } else {
                Entity entity = freed_list.back();
                freed_list.pop_back();
                return entity;
            }
        }
        constexpr void destroy_entity(Entity entity) {
            remove_all(entity);
            entity = entity_traits_v::increment_generation(entity);
            freed_list.push_back(entity);
        }

        constexpr void remove_all(Entity entity) {
            (set<Components>().remove(entity), ...);
        }

        template<typename ComponentType>
        constexpr bool insert(Entity entity, const std::remove_cvref_t<ComponentType>& value) {
            static_assert(is_component<ComponentType>());
            return set<ComponentType>().insert(entity, value);
        }

        template<typename ComponentType>
        constexpr bool remove(Entity entity) {
            static_assert(is_component<ComponentType>());
            return set<ComponentType>().remove(entity);
        }

        template<typename ComponentType>
        constexpr std::remove_cvref_t<ComponentType>* try_get(Entity entity) {
            static_assert(is_component<ComponentType>());
            return set<ComponentType>().try_get(entity);
        }

        template<typename ComponentType>
        constexpr const std::remove_cvref_t<ComponentType>* try_get(Entity entity) const {
            static_assert(is_component<ComponentType>());
            return set<ComponentType>().try_get(entity);
        }

        template<typename ComponentType>
        constexpr bool contains(Entity entity) const {
            static_assert(is_component<ComponentType>());
            return set<ComponentType>().contains(entity);
        }



        template<typename... Inc, typename... Exc>
        constexpr auto view(TypeList<Exc...> _exclude = exclude<>) {
            static_assert(are_components<Inc...>());
            static_assert(are_components<Exc...>());
            static_assert(is_unique_v<Inc..., Exc...>);

            using view_type = sparse_view<
                TypeList<sparse_set<std::remove_cvref_t<Inc>>...>,
                TypeList<sparse_set<std::remove_cvref_t<Exc>>...>
            >;

            if constexpr (sizeof...(Exc) >= 1) {
                return view_type(set<Inc>()..., set<Exc>()...);
            } else {
                return view_type(set<Inc>()...);
            }
        }

        template<typename... Inc, typename... Exc>
        constexpr auto view(TypeList<Exc...> _exclude = exclude<>) const {
            static_assert(are_components<Inc...>());
            static_assert(are_components<Exc...>());
            static_assert(is_unique_v<Inc..., Exc...>);

            using view_type = sparse_view<
                TypeList<const sparse_set<std::remove_cvref_t<Inc>>...>,
                TypeList<const sparse_set<std::remove_cvref_t<Exc>>...>
            >;

            if constexpr (sizeof...(Exc) >= 1) {
                return view_type(set<Inc>()..., set<Exc>()...);
            } else {
                return view_type(set<Inc>()...);
            }
        }
        
    private:
        std::tuple<set_type<Components>...> sets;
        std::vector<Entity> freed_list;
        size_type entity_index = 0;

        template<typename Tp>
        static constexpr bool is_component() {
            return (std::is_same_v<std::remove_cvref_t<Tp>, std::remove_cvref_t<Components>> || ...);
        }

        template<typename... Types>
        static constexpr bool are_components() {
            return (is_component<Types>() && ...);
        }

        template<typename Tp>
        constexpr set_type<Tp>& set() {
            return std::get<set_type<Tp>>(sets);
        }

        template<typename Tp>
        constexpr const set_type<Tp>& set() const {
            return std::get<set_type<Tp>>(sets);
        }
};