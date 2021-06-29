#include <client/graphical/TextureOpener.hpp>
#include <common/logger.hpp>
#include <cstring>

using namespace familyline::graphics;

static bool isDevilOn = false;

TextureFile* TextureOpener::TextureOpenBMP(FILE* f, const char* path)
{
    TextureFile* t = nullptr;
    auto& log      = LoggerService::getLogger();

    char bmp_header[14];
    if (fread((void*)bmp_header, 1, 14, f) < 14) {
        log->write(
            "texture-opener", LogType::Warning, "Unexpected EOF in BMP header while opening %s",
            path);
        return nullptr;
    }

    if (bmp_header[0] != 'B' && bmp_header[1] != 'M') {
        log->write(
            "texture-opener", LogType::Warning, "Invalid magic number while opening %s", path);
        return nullptr;
    }

    unsigned off_pixels = *(unsigned int*)&bmp_header[0x0a];

    if (off_pixels == 0) {
        log->write("texture-opener", LogType::Warning, "Wrong pixel header while opening %s", path);
        return nullptr;
    }

    char dib_header[40];
    if (fread((void*)dib_header, 1, 40, f) < 40) {
        log->write(
            "texture-opener", LogType::Warning, "Unexpected EOF in DIB header while opening %s",
            path);
        return nullptr;
    }

    unsigned hsize, width = 0, height = 0, bpp = 0;
    hsize = *(unsigned int*)&dib_header[0];

    if (hsize <= 12) {
        width  = *(unsigned short*)&dib_header[4];
        height = *(unsigned short*)&dib_header[6];
        bpp    = *(unsigned short*)&dib_header[10];
    } else {
        width  = *(unsigned int*)&dib_header[4];
        height = *(unsigned int*)&dib_header[8];
        bpp    = *(unsigned short*)&dib_header[14];

        if (dib_header[16] != 0) {
            log->write(
                "texture-opener", LogType::Warning,
                "%s uses BMP compression, which is not supported", path);
            return nullptr;
        }
    }

    if (width == 0 || height == 0 || bpp == 0) {
        log->write("texture-opener", LogType::Warning, "Invalid size while opening %s", path);
        return nullptr;
    }

    LOGDEBUG(log, "texture-opener", "image size %ux%ux%u\n", width, height, bpp);

    /* Read the rows */
    const int bytespp = bpp / 8;

    // The row size of a BMP file
    size_t row_size  = (size_t)floor(((bpp * width + 31) / 32.0)) * 4.0;
    auto* image_grid = new unsigned char[width * height * bytespp];
    fseek(f, off_pixels, SEEK_SET);

    for (size_t y = 0; y < height; y++) {
        /* We read row by row, because the each BMP row is padded by a multiple
           of 4. */
        char* row = new char[row_size];
        if (fread((void*)row, 1, row_size, f) < row_size) {
            LOGDEBUG(log, "texture-opener", "error reading row %zu", y);
            return t;
        }

        for (size_t x = 0; x < width; x++) {
            /* BMP stores image upside down.
               Here we rotate back to the normal orientation, hence the (height-y-1) */

            image_grid[(height - y - 1) * width * bytespp + x * bytespp]     = row[x * bytespp];
            image_grid[(height - y - 1) * width * bytespp + x * bytespp + 1] = row[x * bytespp + 1];
            image_grid[(height - y - 1) * width * bytespp + x * bytespp + 2] = row[x * bytespp + 2];
        }

        delete[] row;
    }

    const GLenum iformat = GL_BGR;  // default format for bitmap images.

    /* Create an image */
    ILuint handle;
    ilGenImages(1, &handle);
    ilBindImage(handle);
    ilTexImage(width, height, 1, 3, iformat, IL_UNSIGNED_BYTE, image_grid);
    t = new TextureFile(handle, iformat);
    ilBindImage(0);

    delete[] image_grid;
    
    fclose(f);
    return t;
}

TextureFile* TextureOpener::OpenFile(const char* path)
{
    auto& log = LoggerService::getLogger();
    log->write("texture-opener", LogType::Info, "Opening %s", path);

    FILE* f = fopen(path, "rb");

    if (!f) {
        log->write(
            "texture-opener", LogType::Warning,
            "File %s not found"
            " (error %d)",
            path, errno);
        return nullptr;
    }

    /* For some reason, BMP files don't load.
       We need to do a fast custom BMP loader, then */
    if (strstr(path, ".bmp") || strstr(path, ".BMP")) {
        return TextureOpenBMP(f, path);
    }

    fclose(f);

    /* Initialize devIL if not */
    if (!isDevilOn) {
        ilInit();
    }

    /*      Initialize a handle for the image and
            open it */
    ILuint handle = 0;
    ilGenImages(1, &handle);
    ilBindImage(handle);

    if (ilLoad(IL_TYPE_UNKNOWN, path) == IL_FALSE) {
        int e = ilGetError();
        const char* estr;

        switch (e) {
            case IL_COULD_NOT_OPEN_FILE:
                estr = "Could not open file";
                break;

            case IL_INVALID_EXTENSION:
            case IL_INVALID_FILE_HEADER:
                estr = "Invalid file format.";
                break;

            case IL_INVALID_PARAM:
                estr = "Unrecognized file.";
                break;

            default:
                char* eestr = new char[128];
                sprintf(eestr, "Unknown error %#x", e);
                estr = eestr;
                break;
        }

        log->write("texture-opener", LogType::Error, "Error '%s' while opening %s", estr, path);
        return nullptr;
    }

    auto format = ilGetInteger(IL_IMAGE_FORMAT);

    /* Clean up image */
    ilBindImage(0);

    /* Returns texture */
    return new TextureFile(handle, format);
}

Texture* TextureOpener::OpenTexture(const char* path)
{
    auto f = TextureOpener::OpenFile(path);

    ilBindImage(f->GetHandle());
    int width  = ilGetInteger(IL_IMAGE_WIDTH);
    int height = ilGetInteger(IL_IMAGE_HEIGHT);
    ilBindImage(0);

    auto t = f->GetTextureCut(0, 0, width, height);
    delete f;
    return t;
}
