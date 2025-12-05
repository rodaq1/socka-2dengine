#pragma once

#include <SDL2/SDL.h>

namespace Engine {

    class Input {
    private:
        Input() = delete;

        static const Uint8* currentKeyState;
        static Uint8* lastKeyState;
        static int numKeys;

        static Uint32 currentMouseState;
        static Uint32 lastMouseState;
        static int mouseX;
        static int mouseY; 
        
    public:
        static void init();
        static void shutdown();
        static void update();

        static bool isKeyDown(SDL_Scancode scancode);
        static bool isKeyPressed(SDL_Scancode scancode);
        static bool isKeyReleased(SDL_Scancode scancode);

        static bool isMouseButtonDown(int button);
        static bool isMouseButtonPressed(int button);
        static bool isMouseButtonReleased(int button);
        
        static SDL_Point getMousePosition();
    };

}