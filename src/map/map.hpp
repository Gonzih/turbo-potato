#pragma once

#include <algorithm>
#include <vector>
#include <utility>
#include <memory>
#include <math.h>
#include <unordered_map>
#include <assert.h>

#include "../random.hpp"
#include "../logging.hpp"
#include "../geometry.hpp"

enum TileType
{
    Wall,
    StairsDown,
    StairsUp,
    Empty,
};

class Tile
{
    public:
        explicit Tile(TileType t): m_type { t } { };
        TileType m_type = TileType::Empty;
        bool m_memoized = false;
    private:
};

const Tile wall_tile { TileType::Wall };

enum LightLevel {
    Invisible,
    Dim,
    Visible
};

class LightMap {
    private:
        std::vector<std::vector<LightLevel>> light_map;

        // Implementation based on this pseudo code http://www.roguebasin.com/index.php?title=Eligloscode
        void calc_fov(float x, float y, int w, int h, Vector2D camera_pos, const std::vector<std::vector<Tile>> &map, int light_radius) {
            int i, tx, ty;
            float ox, oy;
            ox = static_cast<float>(camera_pos.x) + 0.5f;
            oy = static_cast<float>(camera_pos.y) + 0.5f;

            for (i = 0; i < light_radius; ++i) {
                tx = static_cast<int>(ox);
                ty = static_cast<int>(oy);

                if (!(tx >= 0 && tx <= w && ty >= 0 && ty <= h)) {
                    return;
                }

                light_map[tx][ty] = LightLevel::Visible;

                if (map[tx][ty].m_type == TileType::Wall) {
                    return;
                }

                ox += x;
                oy += y;
            }
        }
    public:
        LightMap() {};
        explicit LightMap(Vector2D camera_pos, int w, int h, const std::vector<std::vector<Tile>> &map, float light_radius)
        : light_map { std::vector<std::vector<LightLevel>>(w, std::vector<LightLevel>(h, LightLevel::Dim)) }
        {
            float x, y, fi;

            for (int i = 0; i < 360; ++i) {
                fi = static_cast<float>(i);
                x = cos(fi*0.01745f);
                y = sin(fi*0.01745f);

                calc_fov(x, y, w, h, camera_pos, map, light_radius);
            }
        };

        bool visible(int x, int y) {
            return light_level(x, y) == LightLevel::Visible;
        }

        LightLevel light_level(int x, int y) {
            assert(x < light_map.size());
            assert(y < light_map[x].size());
            return light_map[x][y];
        };
};

class Map {
    private:
        int width;
        int height;
        std::vector<std::vector<Tile>> map;
        int nrect = rng::gen_int(12, 26);
        std::vector<Rect> rects;

        Rect gen_rect(int size_w_limit, int size_h_limit) {
            int size_w = rng::gen_int(3, size_w_limit);
            int size_h = rng::gen_int(3, size_h_limit);
            Rect rect;

            // start of rect can be anywhere on screen
            rect.x0 = rng::gen_int(0, width-size_w);
            rect.y0 = rng::gen_int(0, height-size_h);
            // add size to starting point
            rect.x1 = rect.x0 + size_w;
            rect.y1 = rect.y0 + size_h;

            return rect;
        }

        void render(Rect rect) {
            // renders the rectangle on the map
            for (int x = rect.x0; x < rect.x1; ++x) {
                for (int y = rect.y0; y < rect.y1; ++y) {
                    map[x][y].m_type = TileType::Empty;
                }
            }
        }

        void add_tunnel_to_existing(Rect new_rect) {
            if (rects.size() > 0) {
                // Decide which rectangle to connect to
                // Rect rect = existing_rects[rng::gen_int(0, existing_rects.size()) - 1]; // FIXATSOMEPOINT: segfaults
                Rect rect = rects.back();

                // Calculate centers and sort
                int centers_x[2] = { center_x(new_rect), center_x(rect) };
                int centers_y[2] = { center_y(new_rect), center_y(rect) };
                // hacky fix this is needed to handle different types of elbows -| and |-
                int tunnel_y_x_index = ((centers_x[0] > centers_x[1]) && (centers_y[0] > centers_y[1])) ? 1 : 0;
                if (centers_x[0] > centers_x[1])
                    std::swap(centers_x[0], centers_x[1]);
                if (centers_y[0] > centers_y[1])
                    std::swap(centers_y[0], centers_y[1]);

                // Make the horizontal part of the tunnel
                Rect tunnel_x;
                tunnel_x.x0 = centers_x[0];
                tunnel_x.y0 = centers_y[0]-1;
                tunnel_x.x1 = centers_x[1];
                tunnel_x.y1 = centers_y[0]+1;
                render(tunnel_x);
                // Make the vertical part of the tunnel
                Rect tunnel_y;
                tunnel_y.x0 = centers_x[tunnel_y_x_index] - 1;
                tunnel_y.y0 = centers_y[0];
                tunnel_y.x1 = centers_x[tunnel_y_x_index] + 1;
                tunnel_y.y1 = centers_y[1];
                render(tunnel_y);
            }
        }

        void add_stairs(Vector2D pos) {
            logger::info("Generated stairs at", pos.x, pos.y);
            map[pos.x][pos.y].m_type = TileType::StairsDown;
        }

        void generate_maze() {
            logger::info("Maze number of rectangles is", nrect);

            for (int i = 0; i < nrect; ++i) {
                auto rect = gen_rect(10, 10);
                // renders the rectangle on the map
                render(rect);
                // connects rectangle to existing rectangles
                add_tunnel_to_existing(rect);
                rects.push_back(rect);
            }
            add_stairs(get_random_empty_coords());
        }

    public:
        explicit Map(int w, int h) :
            width { w },
            height { h },
            map { std::vector<std::vector<Tile>>(w, std::vector<Tile>(h, wall_tile)) }
        {
            logger::info("Generating maze");
            generate_maze();
        }

        const int get_w() const { return width; }
        const int get_h() const { return height; }
        const TileType at(int x, int y) const { return map[x][y].m_type; }
        const bool memoized(int x, int y) const { return map[x][y].m_memoized; }
        void memoize(int x, int y) { map[x][y].m_memoized = true; }

        Vector2D get_random_empty_coords() const
        {
            int x, y;
            for (;;) {
                x = rng::gen_int(0, width);
                y = rng::gen_int(0, height);
                if (at(x, y) == TileType::Empty) {
                    return Vector2D(x, y);
                }
            }
        }

        bool can_move(Vector2D pos, MovementDirection direction) const
        {
            switch(direction) {
                case MovementDirection::Up:
                    pos.y -= 1;
                    break;
                case MovementDirection::Down:
                    pos.y += 1;
                    break;
                case MovementDirection::Left:
                    pos.x -= 1;
                    break;
                case MovementDirection::Right:
                    pos.x += 1;
                    break;
                case MovementDirection::None:
                    break;
            };

            auto [x, y] = pos;

            return x >= 0 && y >= 0 && x <= width && y <= height && map[x][y].m_type != TileType::Wall;
        };

        std::unique_ptr<LightMap> generate_light_map(Vector2D camera_pos, int light_radius) {
            return std::make_unique<LightMap>(camera_pos, width, height, map, light_radius);
        };
};
