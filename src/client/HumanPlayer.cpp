#include <client/HumanPlayer.hpp>
#include <client/graphical/meshopener/OBJOpener.hpp>
#include <common/logic/logic_service.hpp>
#include <common/Log.hpp>

#include <client/input/input_service.hpp>

using namespace familyline;
using namespace familyline::graphics;
using namespace familyline::logic;
using namespace familyline::input;


#include <cstdio>

bool front = false, back = false;
bool left = false, right = false;
bool rotate_left = false, rotate_right = false;
bool mouse_click = false;
bool exit_game = false;

bool attack_set = false, attack_ready = false;


bool remove_object = false;

bool zoom_in = false;
bool zoom_out = false;
bool zoom_mouse = false;
bool build_something = false;
bool build_tent = false, build_tower = false;

std::weak_ptr<GameObject> attacker, attackee;


HumanPlayer::HumanPlayer(const char* name, int xp, GameActionManager* gam)
    : Player(name, xp, gam)
{
    /* Initialize input subsystems */
	srand((size_t)name*xp);

    _listener = [&](HumanInputAction hia) {
        if (std::holds_alternative<KeyAction>(hia.type)) {
            auto event = std::get<KeyAction>(hia.type);

            switch (event.keycode) {
            case SDLK_w:
                if (event.isPressed)
                    front = true;
                else
                    front = false;
                break;
            case SDLK_s:
                if (event.isPressed)
                    back = true;
                else
                    back = false;
                break;
            case SDLK_a:
                if (event.isPressed)
                    left = true;
                else
                    left = false;
                break;
            case SDLK_d:
                if (event.isPressed)
                    right = true;
                else
                    right = false;
                break;
            case SDLK_LEFT:
                if (event.isPressed)
                    rotate_left = true;
                else
                    rotate_left = false;
                break;
            case SDLK_RIGHT:
                if (event.isPressed)
                    rotate_right = true;
                else
                    rotate_right = false;
                break;

            case SDLK_c:
            {
                if (!event.isPressed)
                    return false;

                build_tent = true;
            }
            break;
            case SDLK_e:
            {
                if (!event.isPressed)
                    return false;

                build_tower = true;
            }
            break;
            case SDLK_r:
            {
                if (!event.isPressed)
                    return false;

                remove_object = true;
            }
            break;

            case SDLK_b: {
                if (!event.isPressed)
                    return false;

                this->renderBBs = !this->renderBBs;
            } break;

            case SDLK_k:
                attack_set = true;
                break;

            case SDLK_PLUS:
            case SDLK_KP_PLUS:
                if (!event.isPressed)
                    zoom_in = false;
                else
                    zoom_in = true;

                break;

            case SDLK_MINUS:
            case SDLK_KP_MINUS:
                if (!event.isPressed)
                    zoom_out = false;
                else
                    zoom_out = true;
                break;
            }
            return true;

        } else if (std::holds_alternative<ClickAction>(hia.type)) {
            auto event = std::get<ClickAction>(hia.type);

            if (attack_set && attack_ready) {
                attack_set = false;
            }
            
            if (event.buttonCode == SDL_BUTTON_LEFT) {
                attack_set = false;
                if (event.isPressed)
                    mouse_click = true;
                else
                    mouse_click = false;
            }

            if (event.buttonCode == SDL_BUTTON_RIGHT && !_selected_obj.expired()) {
                if (event.isPressed) {

                    if (!attack_set) {
                        attack_ready = false;

                        /* Move the object to some position */
                        glm::vec2 to = _ip->GetGameProjectedPosition();
                        auto slock = _selected_obj.lock();

                        auto path = _pf->CreatePath(*slock.get(), to);
                        glm::vec2 lp = path.back();
                        Log::GetLog()->InfoWrite("human-player", "moved to %.2fx%.2f",
                                                 lp.x, lp.y);

                        ObjectPathManager::getInstance()->AddPath(slock.get(), path);

                        this->RegisterMove(slock.get(), to);
                        _updated = true;
                    } else {
                        attack_ready = true;
                        attack_set = true;
                    }
                }

                return true;
            }
            
        } else if (std::holds_alternative<WheelAction>(hia.type)) {
            auto event = std::get<WheelAction>(hia.type);

            if (event.scrollY > 0) {
                zoom_in = true;
                zoom_out = false;
                zoom_mouse = true;
            }

            if (event.scrollY <= 0) {
                zoom_out = true;
                zoom_in = false;
                zoom_mouse = true;
            }

            return true;
        } else if (std::holds_alternative<GameExit>(hia.type)) {
            exit_game = true;
            return true;
        }
        

        return false;

    };

    input::InputService::getInputManager()->addListenerHandler(_listener);

}

void HumanPlayer::SetCamera(familyline::graphics::Camera* c) { _cam = c;}
void HumanPlayer::SetPicker(familyline::input::InputPicker* ip) { _ip = ip; }
void HumanPlayer::SetPathfinder(familyline::logic::PathFinder* p) { _pf = p; }

void HumanPlayer::SetGameActionManager(familyline::logic::GameActionManager* gam)
{
    this->_gam = gam;

    /* Create a city for it */
    City* c = new City{this, glm::vec3(1, 0, 0)};
    AddCity(c);
}


/***
    Virtual function called on each iteration.

    It allows player to decide its movement
    (input for humans, AI decisions for AI... )

    Returns true to continue its loop, false otherwise.
***/
bool HumanPlayer::ProcessInput()
{
    _updated = false;
    auto& ima = InputService::getInputManager();

    ima->processEvents();

    if (zoom_mouse) {
        zoom_in = false;
        zoom_out = false;
        zoom_mouse = false;
    }

    return true;
}

bool HumanPlayer::Play(GameContext* gctx)
{
    if (exit_game)
        return false;

    float unit = 5.0f * gctx->elapsed_seconds;
    float rot_sin = glm::sin(_cam->GetRotation());
    float rot_cos = glm::cos(_cam->GetRotation());

    if (front)
        _cam->AddMovement(glm::vec3
                          (-(unit*rot_sin), 0, -(unit*rot_cos)));
    else if (back)
        _cam->AddMovement(glm::vec3(
                              unit*rot_sin, 0, unit*rot_cos));

    if (left)
        _cam->AddMovement(glm::vec3(
                              -(unit*rot_cos), 0, (unit*rot_sin)));
    else if (right)
        _cam->AddMovement(glm::vec3(
                              unit*rot_cos, 0, -(unit*rot_sin)));

    if (rotate_left)
        _cam->AddRotation(glm::vec3(0, 1, 0), glm::radians(1.0f));
    else if (rotate_right)
        _cam->AddRotation(glm::vec3(0, 1, 0), glm::radians(-1.0f));

    if (mouse_click && build_something) {

        if (BuildQueue::GetInstance()->getNext()) {
            glm::vec3 p = ::GraphicalToGameSpace(_ip->GetTerrainProjectedPosition());

            auto build = BuildQueue::GetInstance()->BuildNext(p);

            BuildQueue::GetInstance()->Clear();
            auto buildpos = build->getPosition();

            buildpos.y = p.y;
            build->setPosition(buildpos);


            // the object will be added to the city
            Log::GetLog()->InfoWrite("human-player",
                                     "creating %s at %.3f %.3f %.3f",
                                     build->getName().c_str(), buildpos.x, buildpos.y, buildpos.z);

            auto cobjID = gctx->om->add(std::move(build));

            auto ncobj =  gctx->om->get(cobjID).value();

            this->RegisterCreation(ncobj.get());

            objr->add(ncobj);
            olm->doRegister(ncobj);

            assert(ncobj->getPosition().x == buildpos.x);
            assert(ncobj->getPosition().z == buildpos.z);

            olm->notifyCreation(cobjID);

            Log::GetLog()->InfoWrite(
                "human-player",
                "%s has id %d now", ncobj->getName().c_str(), cobjID);

        }
    }



    auto l = _ip->GetIntersectedObject().lock();
    if (l) {
        if (mouse_click) {
            _selected_obj = std::dynamic_pointer_cast<GameObject>(l);
        }
        //printf("intersected with %s\n", l->GetName());
    } else {
        if (mouse_click)   _selected_obj = std::weak_ptr<GameObject>();
    }

    auto& of = LogicService::getObjectFactory();

    if (build_tent) {
        printf("Tent built\n");

        auto nobj = std::dynamic_pointer_cast<GameObject>(
            of->getObject("tent", 0, 0, 0));

        if (!nobj) {
            Log::GetLog()->Fatal("human-player", "Type 'tent' has not been found in the object factory!");
        }

        BuildQueue::GetInstance()->Add(nobj);

        build_tent = false;
        build_something = true;
    }

    if (build_tower) {
        auto nobj = std::dynamic_pointer_cast<GameObject>(
            of->getObject("watchtower", 0, 0, 0));
        if (!nobj) {
            Log::GetLog()->Fatal("human-player", "Type 'watchtower' has not been found in the object factory!");
        }

        BuildQueue::GetInstance()->Add(nobj);

        build_tower = false;
        build_something = true;
    }

    if (remove_object) {
        auto l = _ip->GetIntersectedObject().lock();
        if (l) {
            printf("Deleting object %s", l->getName().c_str());
            this->RegisterDestroy(l.get());
            gctx->om->remove(l->getID());
        }
        remove_object = false;
    }

    if (attack_ready) {
        attack_ready = false;

        auto l = _ip->GetIntersectedObject().lock();
        if (l && attack_set)
            attackee = std::dynamic_pointer_cast<GameObject>(l);

        if (!attackee.expired()) {

            if (attack_set) {
                attacker = std::dynamic_pointer_cast<GameObject>(_selected_obj.lock());
            }

            if (!attacker.expired()) {

                auto atkl = attacker.lock();
                auto defl = attackee.lock();

                if (atkl->getAttackComponent() && defl->getAttackComponent()) {
                    this->RegisterAttack(atkl.get(), defl.get());

                    auto& atkManager = LogicService::getAttackManager();
                    atkManager->doRegister(
                        atkl->getID(), atkl->getAttackComponent().value());
                    atkManager->doRegister(
                        defl->getID(), defl->getAttackComponent().value());
                    atkManager->startAttack(
                        atkl->getID(), defl->getID());

                }

            }

        } else {
            attack_ready = false;
            attacker = attackee = std::weak_ptr<GameObject>();
        }
    }

    if (zoom_in || zoom_out) {
        float zfac = (gctx->elapsed_seconds * 0.25);

        if (zoom_mouse)
            zfac *= 5;

        float z = _cam->GetZoomLevel();

        if (zoom_in)
            z += zfac;

        if (zoom_out)
            z -= zfac;

        _cam->SetZoomLevel(z);
    }


    return true;

}



GameObject* HumanPlayer::GetSelectedObject()
{
    return (_selected_obj.expired() ? nullptr : _selected_obj.lock().get());
}

bool HumanPlayer::HasUpdatedObject()
{
    return _updated;
}

HumanPlayer::~HumanPlayer()
{

}