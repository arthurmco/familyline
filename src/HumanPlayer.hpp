/***
    Definitions for human player

    Copyright (C) 2016 Arthur M

***/

#include "logic/Player.hpp"
#include "graphical/Camera.hpp"
#include "logic/ObjectRenderer.hpp"
#include "input/InputPicker.hpp"
#include "input/InputManager.hpp"

#ifndef HUMAN_PLAYER
#define HUMAN_PLAYER

    class HumanPlayer : public Tribalia::Logic::Player
    {
    private:
        Tribalia::Graphics::Camera* _cam;
		Tribalia::Input::InputPicker* _ip;

        Tribalia::Logic::LocatableObject* _selected_obj = nullptr;

    public:
		bool renderBBs = false;

        Tribalia::Logic::ObjectRenderer* objr;

        HumanPlayer(const char* name, int elo=0, int xp=0);

        /***
            Virtual function called on each iteration.

            It allows player to decide its movement
            (input for humans, AI decisions for AI... )

            Returns true to continue its loop, false otherwise.
        ***/
        virtual bool Play(Tribalia::Logic::GameContext*);

        void SetCamera(Tribalia::Graphics::Camera*);
		void SetPicker(Tribalia::Input::InputPicker* ip);
        void SetInputManager(Tribalia::Input::InputManager*);

        Tribalia::Logic::LocatableObject* GetSelectedObject();
        ~HumanPlayer();
    };

#endif /* end of include guard: HUMAN_PLAYER */