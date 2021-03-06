#pragma once

#include <functional>

#include "../geometry.hpp"

enum class MovementDirection {
    None, Up, Down, Left, Right
};

inline MovementDirection opposite_direction(MovementDirection direction)
{
    switch(direction)
    {
        case MovementDirection::Up:
            return MovementDirection::Down;
            break;
        case MovementDirection::Down:
            return MovementDirection::Up;
            break;
        case MovementDirection::Left:
            return MovementDirection::Right;
            break;
        case MovementDirection::Right:
            return MovementDirection::Left;
            break;
        case MovementDirection::None:
            return MovementDirection::None;
            break;
    }
};

namespace ecs::components {
    class TransformComponent;
    class SpriteRenderComponent;
    class MovementComponent;

    using SetPosLambda = std::function<void(Vector2D)>;
    using GetPosLambda = std::function<Vector2D()>;
    using CanMoveLambda = std::function<bool(Vector2D, MovementDirection)>;
    using VisibleLambda = std::function<bool(int, int)>;
    using MemoizedLambda = std::function<bool(int, int)>;
    using GetTextLambda = std::function<std::string()>;
};

#include "transform.hpp"
#include "sprite.hpp"
#include "sprite_render.hpp"
#include "movement.hpp"
#include "darkness.hpp"
#include "text.hpp"
#include "text_render.hpp"
#include "offset.hpp"
