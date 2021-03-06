#pragma once

#include "logging.hpp"
#include "geometry.hpp"
#include "sdl/sdl.hpp"
#include "ecs/ecs.hpp"
#include "components/components.hpp"
#include "map/map.hpp"

using namespace ecs;
using namespace ecs::components;

class Game
{
private:
    bool m_is_running = true;
    int m_difficulty = 0;
    int m_sprite_size = 32;
    int m_light_radius = 15;
    int m_screen_width;
    int m_screen_height;
    int m_playfield_width;
    int m_playfield_height;
    int m_map_width;
    int m_map_height;
    std::shared_ptr<sdl::Window> m_window;
    System m_system;
    std::shared_ptr<Group> m_tiles_group;
    std::shared_ptr<Group> m_player_group;
    std::shared_ptr<Group> m_enemies_group;
    std::shared_ptr<Group> m_darkness_group;

    std::shared_ptr<Entity> player;
    std::shared_ptr<Entity> offset;
    std::shared_ptr<Entity> darkness;

    std::unique_ptr<sdl::SpriteManager> m_sprite_manager;
    std::unique_ptr<Map> m_level;
    std::unique_ptr<LightMap> m_light_map;

public:
    Game(int screen_width, int screen_height) :
        m_screen_width { screen_width },
        m_screen_height { screen_height },
        m_playfield_width { m_screen_width / m_sprite_size },
        m_playfield_height { m_screen_height / m_sprite_size },
        m_map_width { 100 },
        m_map_height { 100 },
        m_window { std::make_shared<sdl::Window>(screen_width, screen_height) },
        m_sprite_manager { std::make_unique<sdl::SpriteManager>(m_window) }
    { };

    void add_map()
    {
        auto wall_sprite = m_sprite_manager->get_sprite("sprites/surroundings.png");
        logger::info("Loading levels sprite");
        m_level = std::make_unique<Map>(m_map_width, m_map_height);
    }

    void add_darkness()
    {
        darkness = m_darkness_group->add_entity();

        auto darkness_sprite = m_sprite_manager->get_sprite("sprites/darkness.png");
        darkness_sprite->set_blend_mode(SDL_BLENDMODE_BLEND);

        darkness->add_component<TransformComponent>(Vector2D { 0, 0 });
        darkness->add_component<MovementComponent>();
        darkness->add_component<SpriteComponent>(m_window, darkness_sprite);
        darkness->add_component<DarknessComponent>(m_map_width, m_map_height, get_visible_fn(), get_memoized_fn(), offset);
    }

    void init()
    {
        m_window->set_resizable(false);
        m_window->open_font("ttf/terminus.ttf", 24);

        m_sprite_manager->preload_sprite("sprites/surroundings.png", 1, 3, m_sprite_size, m_sprite_size);
        m_sprite_manager->preload_sprite("sprites/darkness.png", 1, 1, m_sprite_size, m_sprite_size);
        m_sprite_manager->preload_sprite("sprites/mage.png", 1, 1, m_sprite_size, m_sprite_size, sdl::RGB { 0xFF, 0, 0xFF });

        m_tiles_group = m_system.add_group();
        m_player_group = m_system.add_group();
        player = m_player_group->add_entity();
        offset = m_player_group->add_entity();

        add_map();
        generate_tiles();

        auto player_sprite = m_sprite_manager->get_sprite("sprites/mage.png");

        player->add_component<SpriteComponent>(m_window, player_sprite);
        player->add_component<SpriteRenderComponent>([](int x, int y){ return true; }, offset);
        player->add_component<TransformComponent>(Vector2D { 0, 0 });
        player->add_component<MovementComponent>();

        offset->add_component<TransformComponent>(Vector2D { 0, 0 });
        offset->add_component<OffsetComponent>(Vector2D { m_playfield_width, m_playfield_height }, Vector2D { m_map_width, m_map_height }, player);

        auto pos = m_level->get_random_empty_coords();
        set_centered_player_pos(pos);

        regen_light_map();

        m_enemies_group = m_system.add_group();
        init_enemies();

        m_darkness_group = m_system.add_group();
        add_darkness();

        auto text = m_darkness_group->add_entity();
        text->add_component<TransformComponent>(Vector2D { 0, 0 });
        text->add_component<TextComponent>(m_window, [this]() { return this->log_debug_info(); }, sdl::RGB { 255, 0, 0 });
        text->add_component<TextRenderComponent>();
    }

    std::string log_debug_info()
    {
        auto ppos = get_real_player_pos();
        auto offpos = offset->get_component<TransformComponent>()->get_pos();
        return "player pos is " + ppos.to_string() + " offset is " + offpos.to_string();
    }

    VisibleLambda get_visible_fn()
    {
        return [this](int x, int y)
        {
            return this->visible(x, y);
        };
    }

    MemoizedLambda get_memoized_fn() {
        return [this](int x, int y)
        {
            return this->memoized(x, y);
        };
    }

    void init_enemies()
    {
        auto player_sprite = m_sprite_manager->get_sprite("sprites/mage.png");

        int n = rng::gen_int(4, 11) + m_difficulty;
        for (int i = 0; i < n; ++i) {
            auto enemy = m_enemies_group->add_entity();
            auto pos =  m_level->get_random_empty_coords();

            enemy->add_component<TransformComponent>(pos);
            enemy->add_component<MovementComponent>();
            enemy->add_component<SpriteComponent>(m_window, player_sprite);
            enemy->add_component<SpriteRenderComponent>(get_visible_fn(), offset);
        }
    }

    void quit()
    {
        exit(0);
    }

    void attempt_to_go_next_level()
    {
        auto pos = get_real_player_pos();
        if (can_go_downstairs(pos))
        {
            go_down_level();

            m_tiles_group->destroy_all();
            m_enemies_group->destroy_all();
            m_system.collect_garbage();

            generate_tiles();
            init_enemies();

            m_system.update();
        }
    }

    void handle_keypress(SDL_Event &event)
    {
        static MovementDirection direction;
        static bool shift_pressed = false;
        switch (event.type)
        {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym)
                {
                    case SDLK_LEFT:
                        direction = MovementDirection::Left;
                        logger::info("KEY LEFT");
                        break;
                    case SDLK_RIGHT:
                        direction = MovementDirection::Right;
                        logger::info("KEY RIGHT");
                        break;
                    case SDLK_UP:
                        direction = MovementDirection::Up;
                        logger::info("KEY UP");
                        break;
                    case SDLK_DOWN:
                        direction = MovementDirection::Down;
                        logger::info("KEY DOWN");
                        break;
                    case SDLK_PERIOD:
                        attempt_to_go_next_level();
                        logger::info("KEY DOWNSTIARS");
                        break;
                    case SDLK_ESCAPE:
                        m_is_running = false;
                        logger::info("exiting");
                        break;
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT:
                        shift_pressed = true;
                        break;
                    case SDLK_SEMICOLON:
                        if (shift_pressed) {
                            // TODO
                            logger::info("Opening command mode");
                        }
                        break;
                    default:
                        logger::info("UNHANDLED KEY", event.key.keysym.sym);
                        break;
                }
                break; // SDL_KEYDOWN
            case SDL_KEYUP:
                logger::info("KEY RELEASED", event.key.keysym.sym);
                switch(event.key.keysym.sym)
                {
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                    case SDLK_UP:
                    case SDLK_DOWN:
                        direction = MovementDirection::None;
                        break;
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT:
                        shift_pressed = false;
                        break;
                }
                break; // SDL_KEYUP
        }

        move(direction);
        regen_light_map();
        m_system.update();

        direction = MovementDirection::None;
    }

    void move(MovementDirection direction)
    {
        auto pos = get_real_player_pos();

        if (can_move(pos, direction))
        {
            player->get_component<MovementComponent>()->move(direction);
        }
    }

    void loop()
    {
        SDL_Event event;
        SDL_StartTextInput();
        while(m_is_running)
        {
            m_system.collect_garbage();
            m_window->reset_viewport();
            m_window->clear();

            m_system.draw();
            m_window->update();

            while (SDL_PollEvent(&event) != 0)
            {
                switch(event.type)
                {
                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                        handle_keypress(event);
                        break;
                    case SDL_QUIT:
                        quit();
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // LEVELS RELATED TOOLING

    Vector2D get_real_player_pos()
    {
        return player->get_component<TransformComponent>()->get_pos();
    }

    void set_centered_player_pos(Vector2D pos)
    {
        set_player_pos(pos);
        offset->get_component<OffsetComponent>()->update();
    }

    void set_player_pos(Vector2D pos)
    {
        logger::info("Setting player at (x, y)", pos.x, pos.y);
        player->get_component<TransformComponent>()->set_pos(pos);
    }

    bool can_move(Vector2D pos, MovementDirection direction) const
    {
        return m_level->can_move(pos, direction);
    }

    bool can_go_downstairs(Vector2D pos) const
    {
        return m_level->at(pos.x, pos.y) == TileType::StairsDown;
    }

    bool visible(int x, int y)
    {
        bool vis = m_light_map->visible(x, y);

        if (vis) { m_level->memoize(x, y); }

        return vis;
    }

    bool memoized(int x, int y)
    {
        return m_level->memoized(x, y);
    }

    void regen_light_map()
    {
        auto pos = get_real_player_pos();
        m_light_map = m_level->generate_light_map(pos, m_light_radius);
    }

    void go_down_level()
    {
        add_map();
        auto pos = m_level->get_random_empty_coords();
        set_centered_player_pos(pos);
        regen_light_map();
    }

    void generate_tiles()
    {
        auto sprite = m_sprite_manager->get_sprite("sprites/surroundings.png");

        int sprite_col;
        for (int x = 0; x < m_level->get_w(); ++x)
        {
            for (int y = 0; y < m_level->get_h(); ++y)
            {
                auto tile = m_level->at(x, y);
                auto entity = m_tiles_group->add_entity();

                switch (tile)
                {
                    case TileType::Wall:
                        sprite_col = 0;
                        break;
                    case TileType::Empty:
                        sprite_col = 1;
                        break;
                    case TileType::StairsDown:
                        sprite_col = 2;
                        break;
                    case TileType::StairsUp:
                        break;
                }

                entity->add_component<TransformComponent>(Vector2D { x, y });
                entity->add_component<SpriteComponent>(m_window, sprite);
                entity->add_component<SpriteRenderComponent>(sprite_col, 0, [](int x, int y){ return true; }, offset);
            }
        }
    }
};
