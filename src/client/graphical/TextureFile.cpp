#include <IL/ilu.h>

#include <client/graphical/TextureOpener.hpp>
#include <common/logger.hpp>

using namespace familyline::graphics;

TextureFile::TextureFile(ILuint handle, GLenum format) : _handle(handle), _format(format) {}

unsigned char* TextureFile::GetTextureRaw(int x, int y, int w, int h)
{
    auto& log = LoggerService::getLogger();

    const auto maxtex = Texture::GetMaximumSize();
    auto lhandle      = _handle;

    if ((unsigned)w > maxtex || (unsigned)h > maxtex) {
        log->write(
            "texture-file", LogType::Warning,
            "The solicited cutting size [%d, %d] from texture handle %d exceeded the max allowed "
            "size for your videocard (%d x %d)",
            w, h, _handle, maxtex, maxtex);

        // Create another handle and bind it
        ilGenImages(1, &lhandle);
        ilBindImage(lhandle);
        ilBlit(_handle, 0, 0, 0, x, y, 0, w, h, 1);

        if (w >= h) {
            auto nw = (maxtex * w) / h;
            h       = maxtex;
            w       = nw;
        } else {
            auto nh = (maxtex * h) / w;
            h       = nh;
            w       = maxtex;
        }

        iluScale(w, h, 1);

        log->write("texture-file", LogType::Warning, "Rescaled it to %d x %d", w, h);
    }

    ilBindImage(lhandle);
    int s = (_format == IL_BGR || _format == IL_RGB) ? 3 : 4;

    if (w < 0) w = ilGetInteger(IL_IMAGE_WIDTH);
    if (h < 0) h = ilGetInteger(IL_IMAGE_HEIGHT);

    char* c = new char[w * h * s];
    log->write(
        "texture-file", LogType::Debug,
        "Got image with handle %d and format 0x%x, cutting [%d, %d, %d, %d]", lhandle, _format, x,
        y, w, h);

    ilCopyPixels(x, y, 0, w, h, 1, _format, IL_UNSIGNED_BYTE, c);
    ilBindImage(0);
    return (unsigned char*)c;
}

Texture* TextureFile::GetTextureCut(int x, int y, int w, int h)
{
    ilBindImage(_handle);
    if (w < 0) w = ilGetInteger(IL_IMAGE_WIDTH);
    if (h < 0) h = ilGetInteger(IL_IMAGE_HEIGHT);
    auto* r = GetTextureRaw(x, y, w, h);
    
    if (r)
        return new Texture(w, h, _format, r);

    return nullptr;
}

ILuint TextureFile::GetHandle() const { return _handle; }

GLenum TextureFile::GetFormat() const { return _format; }

TextureFile::~TextureFile() { ilDeleteImage(_handle); }
