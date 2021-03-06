#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cassert>
#include <optional>
#include <memory>
#include <unordered_map>
#include <string>

namespace sdl
{

    inline void init_sdl()
    {
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            throw std::runtime_error(strcat(strdup("SDL could not initialize! SDL_Error: "), SDL_GetError()));
        }

        if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
        {
            printf("Warning: Linear texture filtering not enabled!");
        }
    }

    inline void init_image()
    {
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags))
        {
            throw std::runtime_error(strcat(strdup("SDL IMAGE could not be initialize! IMG_Error: "), IMG_GetError()));
        }
    }

    inline void init_ttf()
    {
        if (TTF_Init() == -1)
        {
            throw std::runtime_error(strcat(strdup("SDL TTF could not be initialize! TTF_Error: "), TTF_GetError()));
        }
    }

    inline void init()
    {
        init_sdl();
        init_image();
        init_ttf();
    }

    inline void quit()
    {
        IMG_Quit();
        SDL_Quit();
    }

    class RGB
    {
        public:
            Uint8 r, g, b, a;
            RGB(Uint8 r, Uint8 g, Uint8 b)
            : r { r }, g { g }, b { b }, a { 255 } { };
            RGB(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
            : r { r }, g { g }, b { b }, a { a } { };
            operator SDL_Color()   { return SDL_Color { r, g, b, a }; }
    };

    class Renderer
    {
        private:
            SDL_Renderer* m_renderer;
        public:
            Renderer() {};
            explicit Renderer(SDL_Window* window, Uint32 flags)
            : m_renderer { SDL_CreateRenderer(window, -1, flags) }
            {
                if (window == NULL)
                {
                    throw std::runtime_error(strcat(strdup("Cant create renderer from NULL window: "), SDL_GetError()));
                }
                if (m_renderer == NULL)
                {
                    throw std::runtime_error(strcat(strdup("Window Renderer failed to init: "), SDL_GetError()));
                }
            }

            SDL_Renderer& operator*()  { return *(m_renderer); }
            operator SDL_Renderer*()   { return m_renderer; }

            virtual ~Renderer()
            {
                if (m_renderer != NULL)
                { SDL_DestroyRenderer(m_renderer); }
            }
    };

    class Surface
    {
        private:
            SDL_Surface* m_surface = NULL;
            bool m_auto_free = true;
            int m_width = 0;
            int m_height = 0;

            void set_dimensions()
            {
                m_width = m_surface->w;
                m_height = m_surface->h;
            }
        public:
            Surface() {}
            explicit Surface(SDL_Surface* surface)
            : m_surface { surface }
            {
                set_dimensions();
            }

            explicit Surface(SDL_Window* window, std::optional<RGB> ock)
            : m_surface { SDL_GetWindowSurface(window) }, m_auto_free { false }
            {
                if (m_surface == NULL)
                {
                    throw std::runtime_error(strcat(strdup("Could not get m_surface from a m_window : "), SDL_GetError()));
                }
                set_dimensions();
                set_color_key(ock);
            }

            explicit Surface(std::string path, std::optional<RGB> ock)
            : m_surface { IMG_Load(path.c_str()) }
            {
                if (m_surface == NULL)
                {
                    throw std::runtime_error(strcat(strcat(strdup("Could not load m_surface from a file: "), path.c_str()), SDL_GetError()));
                }
                set_dimensions();
                set_color_key(ock);
            }

            int get_w() { return m_width; };
            int get_h() { return m_height; };

            virtual ~Surface()
            {
                if (m_surface != NULL && m_auto_free)
                {
                    SDL_FreeSurface(m_surface);
                    m_surface = nullptr;
                }
            }

            SDL_Surface& operator*()  { return *(m_surface); }
            operator SDL_Surface*()   { return m_surface; }

            int blit(Surface& source)
            {
                return SDL_BlitSurface(source, NULL, m_surface, NULL);
            }

            Surface optimize(SDL_PixelFormat* format)
            {
                return Surface { SDL_ConvertSurface(m_surface, format, 0) };
            }

            void set_color_key(std::optional<RGB> ock)
            {
                if (ock.has_value())
                {
                    auto ck = ock.value();
                    SDL_SetColorKey(m_surface, SDL_TRUE, SDL_MapRGB(m_surface->format, ck.r, ck.g, ck.b));
                }
            }
    };

    class Texture
    {
        private:
            int m_height = 0;
            int m_width = 0;
            SDL_Texture* m_texture;

            void set_dimensions(Surface& surface)
            {
                m_width = surface.get_w();
                m_height = surface.get_h();
            }

            SDL_Rect render_rect(int x, int y)
            {
                SDL_Rect rect;
                rect.x = x;
                rect.y = y;
                rect.w = m_width;
                rect.h = m_height;

                return rect;
            }
        public:
            Texture() {}

            explicit Texture(std::string path, Renderer& renderer, std::optional<RGB> ock)
            : Texture { Surface { path, ock }, renderer }
            { }

            explicit Texture(Surface&& surface, Renderer& renderer)
            {
                set_dimensions(surface);
                m_texture = SDL_CreateTextureFromSurface(renderer, surface);
            }

            SDL_Texture& operator*()  { return *(m_texture); }
            operator SDL_Texture*()   { return m_texture; }

            int get_w() { return m_width; };
            int get_h() { return m_height; };

            void render(SDL_Renderer* renderer, int x, int y, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE)
            {
                auto renderQuad = render_rect(x, y);
                SDL_RenderCopyEx(renderer, m_texture, NULL, &renderQuad, angle, center, flip);
            }

            void set_color_mod(RGB rgb)
            {
                SDL_SetTextureColorMod(m_texture, rgb.r, rgb.g, rgb.b);
            }

            void set_blend_mode(SDL_BlendMode blending)
            {
                SDL_SetTextureBlendMode(m_texture, blending);
            }

            void set_alpha(Uint8 alpha)
            {
                SDL_SetTextureAlphaMod(m_texture, alpha);
            }

            virtual ~Texture()
            {
                if (m_texture != NULL)
                {
                    SDL_DestroyTexture(m_texture);
                    m_texture = nullptr;
                }
            }
    };

    class Sprite
    {
        private:
            Texture m_texture;
            int m_rows = 0;
            int m_cols = 0;
            int m_width = 0;
            int m_height = 0;

            SDL_Rect clip_rect(int col, int row)
            {
                SDL_Rect rect;
                rect.x = m_width * col;
                rect.y = m_height * row;
                rect.w = m_width;
                rect.h = m_height;

                return rect;
            }

            SDL_Rect render_rect(int x, int y)
            {
                SDL_Rect rect;
                rect.x = x;
                rect.y = y;
                rect.w = m_width;
                rect.h = m_height;

                return rect;
            }
        public:
            Sprite() {}

            Sprite(std::string path, Renderer& renderer, int rows, int cols, int width, int height, std::optional<RGB> ock)
            : m_texture { path, renderer, ock },
              m_rows { rows }, m_cols { cols },
              m_width { width }, m_height { height }
            { }

            void render(SDL_Renderer* renderer, int col, int row, int x, int y, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE)
            {
                assert(col < m_cols);
                assert(row < m_rows);

                auto clip = clip_rect(col, row);
                auto renderQuad = render_rect(x, y);

                SDL_RenderCopyEx(renderer, m_texture, &clip, &renderQuad, angle, center, flip);
            }

            void set_color_mod(RGB rgb)
            {
                m_texture.set_color_mod(rgb);
            }

            void set_blend_mode(SDL_BlendMode blending)
            {
                m_texture.set_blend_mode(blending);
            }

            void set_alpha(Uint8 alpha)
            {
                m_texture.set_alpha(alpha);
            }

            int get_w() const
            { return m_width; }

            int get_h() const
            { return m_height; }
    };

    class Window
    {
        private:
            int m_width;
            int m_height;
            SDL_Window* m_window = NULL;
            Renderer m_renderer;
            TTF_Font* m_font = NULL;

        public:
            Window(int w, int h)
            : m_width { w }, m_height { h },
              m_window { SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN) },
              m_renderer { m_window, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC }
            { }

            void reset_viewport()
            {
                set_viewport(0, 0, m_width, m_height);
            }

            void clear()
            {
                set_draw_color(0xFF, 0xFF, 0xFF, 0xFF);
                SDL_RenderClear(m_renderer);
            }

            void update()
            {
                SDL_RenderPresent(m_renderer);
            }

            Renderer& get_renderer()
            {
                return m_renderer;
            }

            [[deprecated]]
            Texture load_texture(std::string path)
            {
                return Texture { path, m_renderer, std::nullopt };
            }

            [[deprecated]]
            Texture load_texture(std::string path, RGB ck)
            {
                return Texture { path, m_renderer, std::optional<RGB> { ck } };
            }

            std::shared_ptr<Texture> render_text(std::string text, RGB color)
            {
                SDL_Surface* sdl_surf = TTF_RenderText_Blended(m_font, text.c_str(), color);
                if (sdl_surf == NULL)
                {
                    throw std::runtime_error("Failed to render text " + text);
                }

                return std::make_shared<Texture>(Surface { sdl_surf }, m_renderer);
            }

            void open_font(std::string path, int point_size)
            {
                 m_font = TTF_OpenFont(path.c_str(), point_size);
                 if (m_font == NULL)
                 {
                    throw std::runtime_error("Could not load font from " + path);
                 }
            }


            void set_viewport(int x, int y, int w, int h)
            {
                SDL_Rect viewport;
                viewport.x = x;
                viewport.y = y;
                viewport.w = w;
                viewport.h = h;
                SDL_RenderSetViewport(m_renderer, &viewport);
            }

            std::pair<int, int> get_screen_size()
            {
                SDL_DisplayMode DM;
                SDL_GetCurrentDisplayMode(0, &DM);

                return std::pair<int, int> { DM.w, DM.h };
            }

            void set_draw_color(int r, int g, int b, int a)
            {
                SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
            }

            void set_resizable(bool v)
            {
                SDL_bool b = v ? SDL_TRUE : SDL_FALSE;
                SDL_SetWindowResizable(m_window, b);
            }

            virtual ~Window()
            {
                if (m_window != NULL)
                { SDL_DestroyWindow(m_window); }
            }
    };

    class SpriteManager
    {
        private:
            std::shared_ptr<Window> m_window;
            std::unordered_map<std::string, std::shared_ptr<Sprite>> m_sprites;
        public:
            SpriteManager(std::shared_ptr<Window> window) : m_window { window }, m_sprites { } { };
            ~SpriteManager() { };

            void preload_sprite(std::string path, int rows, int cols, int width, int height)
            {
                auto sprite = std::make_shared<Sprite>(path, m_window->get_renderer(), rows, cols, width, height, std::nullopt);
                m_sprites[path] = sprite;
            }

            void preload_sprite(std::string path, int rows, int cols, int width, int height, RGB ck)
            {
                auto sprite = std::make_shared<Sprite>(path, m_window->get_renderer(), rows, cols, width, height, std::optional<RGB>{ ck });
                m_sprites[path] = sprite;
            }

            std::shared_ptr<Sprite> get_sprite(std::string path)
            {
                if (m_sprites.find(path) == m_sprites.end())
                    throw std::runtime_error("Could not find sprite for " + path);

                return m_sprites[path];
            }
    };
}
