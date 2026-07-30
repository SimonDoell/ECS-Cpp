#pragma once
#include "SparseSet.hpp"
#include "SparseView.hpp"


template<typename... Components>
struct ecs {
        using size_type = uint32_t;

        static constexpr size_type component_count = sizeof...(Components);
        
    
    public:
        ecs() {}

        template<typename... Include>
        constexpr sparse_view<Include...> view() {}

    private:
        
};