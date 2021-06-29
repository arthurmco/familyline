/***
    Texture manager

    Copyright (C) 2016, 2018-2019 Arthur Mendes.

***/
#pragma once

#include <cstring>  //strcmp()
#include <map>
#include <string>

#include "Texture.hpp"

namespace familyline::graphics
{

    // TODO: make this use an unique_ptr
class TextureManager
{
private:
    std::map<std::string, Texture*> _textures;

    static TextureManager* _tm;

public:
    /* Add texture, return its ID */
    int AddTexture(const char* name, Texture*);

    Texture* GetTexture(int ID);
    Texture* GetTexture(const char* name);

    ~TextureManager() {
        for (auto& [n, ptr] : _textures) {
            delete ptr;
        }
    }
};

}  // namespace familyline::graphics
