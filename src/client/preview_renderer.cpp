#include <client/graphical/camera.hpp>
#include <client/input/InputPicker.hpp>
#include <client/input/input_service.hpp>
#include <client/preview_renderer.hpp>
#include <common/logic/object_factory.hpp>
#include <memory>

using namespace familyline;
using namespace familyline::input;
using namespace familyline::logic;

PreviewRenderer::PreviewRenderer(ObjectFactory& of, graphics::Renderer& rndr, InputPicker& ip)
    : of_(of), rndr_(rndr), ip_(ip)
{
    lh_ = input::InputService::getInputManager()->addListenerHandler(
        [&](HumanInputAction hia) {
            if (std::holds_alternative<MouseAction>(hia.type)) {
                auto event = std::get<MouseAction>(hia.type);

                this->coords_ = this->getMouseCoords(glm::vec2(event.screenX, event.screenY));
            }

            return false;
        },
        2);

    rndr_hook_ = rndr.addHook([&](graphics::Camera* c) {
        if (prev_data_ && coords_ && this->enable_) {
            auto& pi = std::get<0>(*prev_data_);
            glm::vec3 fixcoords =
                glm::vec3(coords_->x, std::min(coords_->y, c->GetPosition().y - 10), coords_->z);

            pi.m->setLogicPosition(fixcoords);
            pi.m->update();
            auto vinfos = pi.m->getVertexInfo();
            for (auto i = 0; i < pi.vhandles.size(); i++) {
                pi.vhandles[i]->vinfo = vinfos[i];
            }
        }
    });
}

PreviewRenderer::~PreviewRenderer()
{
    input::InputService::getInputManager()->removeListenerHandler(lh_);
    rndr_.removeHook(rndr_hook_);
}

/// Set the previewed building to the one of type `type`.
///
/// Returns true if the entity exists and is visible, false if neither
/// exists nor is visible.
///
/// Also sets if we can build the object. We pass this here, because it is
/// only a renderer, not a class that will detect if the building can be
/// placed.
bool PreviewRenderer::add(std::string type, bool possible)
{
    this->reset();
    auto o = of_.getObject(type.c_str(), 0, 0, 0);

    if (o) {
        if (auto c = o->getLocationComponent(); c) {
            glm::vec3 ncoords = (coords_) ? *coords_ : glm::vec3(0, -128, 0);
            
            auto mesh = std::dynamic_pointer_cast<graphics::Mesh>(c->mesh);
            mesh->setLogicPosition(ncoords);
            mesh->update();

            auto vdata = mesh->getVertexData();
            auto vinfo = mesh->getVertexInfo();
            std::vector<graphics::VertexHandle*> vhandles;

            for (auto i = 0; i < vdata.size(); i++) {
                vhandles.push_back(rndr_.createVertex(vdata[i], vinfo[i]));
            }

            prev_data_ = std::make_tuple(PreviewInfo{mesh, vhandles}, possible);
        }
        
        enable_ = true;
    }

    return enable_;
}

/// Reset the preview, disabling it
void PreviewRenderer::reset()
{
    if (prev_data_) {
        auto& pi = std::get<0>(*prev_data_);
        for (auto& v : pi.vhandles) {
            rndr_.removeVertex(v);
        }
    }

    prev_data_ = std::nullopt;
    coords_    = std::nullopt;
    enable_    = false;
}

/// Private, get the mouse coordinates (they will come from a callback) and
/// plot them.
glm::vec3 PreviewRenderer::getMouseCoords(glm::vec2 pos) const
{
    return ip_.GetTerrainProjectedPosition();
}
