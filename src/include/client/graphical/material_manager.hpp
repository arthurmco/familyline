/***
    Material manager

    Copyright (C) 2016, 2018-2019 Arthur Mendes.

***/
#pragma once

#include <cstring>  //strcmp()
#include <vector>

#include "material.hpp"

namespace familyline::graphics
{
class MaterialManager
{
private:
    std::vector<Material> _materials;

    static MaterialManager* _mm;

public:
    /* Add material, return its ID */
    int addMaterial(Material*);

    /* Add materials*/
    void addMaterials(std::vector<Material*>);

    Material* getMaterial(int ID);
    Material* getMaterial(const char* name);

};

}  // namespace familyline::graphics
