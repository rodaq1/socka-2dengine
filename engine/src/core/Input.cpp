#include "Input.h"
#include <cstring>

namespace Engine {

    const Uint8* Input::currentKeyState = nullptr;
    Uint8* Input::lastKeyState = nullptr;
    int Input::numKeys = 0;

    Uint32 Input::currentMouseState = 0;
    Uint32 Input::lastMouseState = 0;
    int Input::mouseX = 0;
    int Input::mouseY = 0;

    void Input::init() {
        currentKeyState = SDL_GetKeyboardState(&numKeys);
        lastKeyState = new Uint8[numKeys];

        //memory copy from source to destinaton
        memcpy(lastKeyState, currentKeyState, numKeys);  
        
        currentMouseState = SDL_GetMouseState(&mouseX, &mouseY);
        lastMouseState = currentMouseState;  
    }

    void Input::shutdown() {
        delete[] lastKeyState;
        lastKeyState = nullptr;
    }

    void Input::update() {
        /*
            Funkcia callnuta na konci framu.
            Kopiruje current state do last state bufferu
            na pripravu pre input checky v dalsom frame
        */

        memcpy(lastKeyState, currentKeyState, numKeys);

        lastMouseState = currentMouseState;

        currentMouseState = SDL_GetMouseState(&mouseX, &mouseY);
    }

    //keyboard queries
    bool Input::isKeyDown(SDL_Scancode scancode) {
        return currentKeyState[scancode]; 
    }

    bool Input::isKeyPressed(SDL_Scancode scancode) {
        return currentKeyState[scancode] && !lastKeyState[scancode];
    }

    bool Input::isKeyReleased(SDL_Scancode scancode) {
        return !currentKeyState[scancode] && lastKeyState[scancode];
    }

    //mouse queries
    bool Input::isMouseButtonDown(int button) {
        return (currentMouseState & SDL_BUTTON(button));
    }

    bool Input::isMouseButtonPressed(int button) {
        return (currentMouseState & SDL_BUTTON(button)) && !(lastMouseState &  SDL_BUTTON(button));
    }

    bool Input::isMouseButtonReleased(int button) {
        return !(currentMouseState & SDL_BUTTON(button)) && (lastMouseState & SDL_BUTTON(button));
    }

    SDL_Point Input::getMousePosition() {
        return {mouseX, mouseY};
    }
}