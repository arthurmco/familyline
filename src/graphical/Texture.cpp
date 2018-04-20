#include "Texture.hpp"



using namespace Tribalia::Graphics;

/* Create a texture from an new image */
Texture::Texture(int width, int height, GLenum format, void* data)
{
    GLuint tex_handle = 0;
    _totalw = _w = width;
    _totalh = _h = height;
    
    // Create, bind and setup texture
    glGenTextures(1, &tex_handle);
    glBindTexture(GL_TEXTURE_2D, tex_handle);

    glGetError();    
    GLenum glerr = GL_NO_ERROR;
    if ((glerr = glGetError()) != GL_NO_ERROR) {
	Log::GetLog()->Warning("texture", "GL error %#x while binding texture handle %d",
			       glerr, tex_handle);
    }
    
    GLenum dest_format = GL_RGB;
    if (format == GL_RGBA || format == GL_BGRA) {
	dest_format = GL_RGBA;
    } 
    
    glTexImage2D(GL_TEXTURE_2D, 0, dest_format, width, height, 0, format,
		 GL_UNSIGNED_BYTE, data);

    glerr = GL_NO_ERROR;
    if ((glerr = glGetError()) != GL_NO_ERROR) {
	Log::GetLog()->Warning("texture", "GL error %#x while creating texture handle %d",
			       glerr, tex_handle);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		    GL_NEAREST);
    Log::GetLog()->Write("texture", "Created texture with %dx%d, format %#x, with id %d",
			 width, height, format, tex_handle);

    
    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
    _tex_handle = tex_handle;
}

GLint Texture::GetHandle() const { return _tex_handle; }


/* Creates a texture from an existing handle (effectively a subtexture) */
Texture::Texture(GLuint handle, int w, int h, glm::vec2 uv_pos,
		 glm::vec2 uv_size)
{
    _tex_handle = handle;
    _totalw = _w = w;
    _totalh = _h = h;
    _area.pos = uv_pos;
    _area.size = uv_size;
    Log::GetLog()->Write("texture",
			 "Created texture from handle %d (%dx%d), cutted with uv-pos (%.2f, %.2f) and uv-size (%.2f, %.2f)", handle, w, h, uv_pos.x, uv_pos.y, uv_size.x, uv_size.y);
}

void Texture::SetName(const char* name)
{
    _name = std::string{name};
}

const char* Texture::GetName() const
{
    return _name.c_str();
}

glm::vec2 Texture::GetAreaPosition(bool clamp)
{
    if (!clamp)
	return glm::vec2(_area.pos.x * _w, _area.pos.y * _h);
    else
	return _area.pos;
}

glm::vec2 Texture::GetAreaSize(bool clamp)
{
    if (!clamp)
	return glm::vec2(_area.size.x * _w, _area.size.y * _h);
    else
	return _area.size;
}

Texture* Texture::GetSubTexture(int xpos, int ypos, int w, int h)
{
    glm::vec2 uv_pos = glm::vec2(xpos / _totalw, ypos / _totalh);
    glm::vec2 uv_size = glm::vec2(w / _totalw, h / _totalh);

    Texture* t = new Texture(_tex_handle, w, h, uv_pos, uv_size);
    t->_totalw = _totalw;
    t->_totalh = _totalh;
    t->_handleowner = false;
    
    Log::GetLog()->Write("texture", "Created a subdivision from texture %s (handle %d) at point (%d, %d) size (%d, %d)", t->_name.c_str(), _tex_handle, xpos, ypos, w, h);
    t->_name = _name.append("_sub");
    return t;
    
}

glm::mat4 Texture::GetOffsetMatrix()
{
    return glm::mat4(
	_area.size.x, 0, 0, _area.pos.x,
	0, _area.size.y, 0, _area.pos.y,
	0, 0, 1, 1,
	0, 0, 0, 1
	);
	
}

/* Get maximum possible size for a texture */
size_t Texture::GetMaximumSize()
{
    auto maxtex = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtex);
    return (size_t)maxtex;
}
