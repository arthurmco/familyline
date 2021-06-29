#include <client/graphical/gfx_service.hpp>

using namespace familyline::graphics;

std::unique_ptr<ShaderManager> GFXService::_shaderm;
std::unique_ptr<AssetManager> GFXService::_assetm;
std::unique_ptr<MaterialManager> GFXService::_materialm;
std::unique_ptr<TextureManager> GFXService::_texturem;
std::unique_ptr<Device> GFXService::_devicem;
