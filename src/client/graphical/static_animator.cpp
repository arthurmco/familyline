#include <client/graphical/static_animator.hpp>

using namespace familyline::graphics;

StaticAnimator::StaticAnimator(VertexDataGroup vdg) : _vdg(vdg) {}

void StaticAnimator::advance(double ms) {}
void StaticAnimator::runAnimation(const char* name) {}

VertexDataGroup StaticAnimator::getCurrentFrame()
{
    this->dirtyFrame = false;
    return _vdg;
}
