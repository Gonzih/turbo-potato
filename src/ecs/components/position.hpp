#pragma once

#include <memory>

#include "../../geometry.hpp"
#include "../ecs.hpp"

namespace ecs::components
{
    class PositionComponent : public Component
    {
    private:
        Point pos;
    public:
        PositionComponent() : pos { 0, 0 } {  };
        PositionComponent(Point p) : pos { p } {  };
        PositionComponent(int x, int y) : pos { x, y } {  };
        virtual ~PositionComponent() override {  };

        void set_pos(Point np) { pos = np; };
        const Point get_pos() { return pos; };
        const int get_x() { return pos.x; };
        const int get_y() { return pos.y; };
    };
};
