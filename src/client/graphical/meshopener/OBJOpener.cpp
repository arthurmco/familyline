#include <algorithm>
#include <client/graphical/exceptions.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/material_manager.hpp>
#include <client/graphical/meshopener/OBJOpener.hpp>
#include <client/graphical/shader_manager.hpp>
#include <client/graphical/static_animator.hpp>
#include <common/logger.hpp>
#include <cstdio>
#include <cstring>
#include <glm/glm.hpp>

using namespace familyline::graphics;

struct FaceIndex {
    unsigned idxVertex[3], idxNormal[3], idxTex[3];
};

/*
 * The unique vertex object
 * Each vertex here will be unique, and will be referenced by the index idx
 *
 */
struct UniqueVertex {
    unsigned idx = -1;

    int idxVertex = -1, idxNormal = -1, idxTexcoord = -1;

    bool inline operator==(const UniqueVertex& b)
    {
        return (
            this->idxVertex == b.idxVertex && this->idxNormal == b.idxNormal &&
            this->idxTexcoord == b.idxTexcoord);
    }
};

// The vertex list
// Represented by the mesh object (the 'o' marker)
struct VertexList {
    // The indices used for building each one of the faces
    // Each element in the face index is a 3-element array
    std::vector<FaceIndex> indices;

    std::string mtlname = "";
};

// The vertex group, aka the mesh
// Represents a mesh group in the file (thie 'g' marker)
struct VertexGroup {
    // The vertex list name
    std::string name;
    bool complete = false;

    bool hasTexture = false, hasNormal = false;
    std::vector<VertexList> vertices;

    VertexGroup(std::string name) { this->name = name; }
};

std::vector<Mesh*> OBJOpener::OpenSpecialized(const char* file)
{
    auto& log = LoggerService::getLogger();

    /* The vertices, normals and texcoords of the file
     *
     * The OBJ file indexes the vertices globally, not per mesh
     * (ex: the vertex index 400 is the 400th vertex of the file, not the mesh )
     */
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    FILE* fObj = fopen(file, "r");
    if (!fObj) {
        char s[512];
        snprintf(s, 511, "Failure to open mesh %s (%d)", file, errno);

        throw asset_exception(s, AssetError::AssetOpenError);
    }

    // TODO: Support meshes with more than 3 vertices per face

    // Setup the vertex group and list lists
    std::vector<VertexGroup> verts;
    verts.push_back(VertexGroup{"default"});
    VertexGroup* current_group = &verts.back();

    current_group->vertices.push_back(VertexList{});
    VertexList* current_vert = &current_group->vertices.back();

    char* line = new char[256];

    while (!feof(fObj)) {
        auto l = fgets(line, 255, fObj);
        if (!l) {
            if (feof(fObj)) break;

            throw std::runtime_error{"Error while reading the file"};
        }

        // Remove whitespace
        while (l[0] == ' ') l++;

        // Empty line
        if (l[0] == '\0') continue;

        if (l[0] == '#')  // Comment
            continue;

        // Remove the newline
        l[strlen(l) - 1] = '\0';

        // Vertex
        if (l[0] == 'v' && l[1] == ' ') {
            glm::vec3 v3;
            char vs;
            auto i = sscanf(l, "%c %f %f %f", &vs, &v3.x, &v3.y, &v3.z);

            if (i < 4)  // Not enough parameters for the vertex.
                continue;

            // Since OBJ files indexes vertices by appearance order, no problem in pushing them
            vertices.push_back(std::move(v3));
            continue;
        }

        // Normal
        if (l[0] == 'v' && l[1] == 'n' && l[2] == ' ') {
            glm::vec3 v3;
            char vs[8];
            auto i = sscanf(l, "%s %f %f %f", vs, &v3.x, &v3.y, &v3.z);

            if (i < 4)  // Not enough parameters for the vertex.
                continue;

            v3 = glm::normalize(v3);

            normals.push_back(std::move(v3));
            current_group->hasNormal = true;
            continue;
        }

        // Texture
        if (l[0] == 'v' && l[1] == 't' && l[2] == ' ') {
            glm::vec2 v2;
            char vs[8];
            auto i = sscanf(l, "%s %f %f", vs, &v2.x, &v2.y);

            if (i < 3)  // Not enough parameters for the vertex.
                continue;

            texcoords.push_back(std::move(v2));
            current_group->hasTexture = true;
            continue;
        }

        // TODO: Support line elements?
        // They might be a mesh with a line shader

        // Vertex list changed
        if (l[0] == 'o') {
            current_group->vertices.push_back(VertexList{});
            current_vert = &current_group->vertices.back();
            continue;
        }

        // TODO: Switch vertex list on material change
        // Vertex group changed
        if (l[0] == 'g') {
            char gs;
            char* gname = new char[strlen(l)];

            auto i = sscanf(l, "%c %s", &gs, gname);
            if (i < 2) {
                delete[] gname;
                continue;
            }

            verts.push_back(VertexGroup{gname});
            current_group = &verts.back();

            // Add a default vertex list, for when the software only adds vertex groups
            current_group->vertices.push_back(VertexList{});
            current_vert = &current_group->vertices.back();

            log->write("meshopener::obj", LogType::Debug, "found group '%s'", gname);

            delete[] gname;            
            continue;
        }

        // Material
        if (l[0] == 'u' && l[1] == 's' && l[2] == 'e') {
            if (!strncmp(l, "usemtl", 6)) {
                std::string mtlname = std::string{&l[7]};

                log->write(
                    "meshopener::obj", LogType::Debug, "group %s: found material '%s'",
                    current_group->name.c_str(), mtlname.c_str());
                // switch vertex list
                current_group->vertices.push_back(VertexList{});
                current_vert          = &current_group->vertices.back();
                current_vert->mtlname = mtlname;

                continue;
            }
        }

        // Face assembling
        if (l[0] == 'f') {
            FaceIndex fi;
            char fs;

            if (current_group->hasNormal && !current_group->hasTexture) {
                auto i = sscanf(
                    l, "%c %d//%d %d//%d %d//%d", &fs, &fi.idxVertex[0], &fi.idxNormal[0],
                    &fi.idxVertex[1], &fi.idxNormal[1], &fi.idxVertex[2], &fi.idxNormal[2]);

                if (i < (2 * 3) + 1) continue;

            } else if (!current_group->hasNormal && current_group->hasTexture) {
                auto i = sscanf(
                    l, "%c %d/%d %d/%d %d/%d", &fs, &fi.idxVertex[0], &fi.idxTex[0],
                    &fi.idxVertex[1], &fi.idxTex[1], &fi.idxVertex[2], &fi.idxTex[2]);

                if (i < (2 * 3) + 1) continue;

            } else if (current_group->hasNormal && current_group->hasTexture) {
                auto i = sscanf(
                    l, "%c %d/%d/%d %d/%d/%d %d/%d/%d", &fs, &fi.idxVertex[0], &fi.idxTex[0],
                    &fi.idxNormal[0], &fi.idxVertex[1], &fi.idxTex[1], &fi.idxNormal[1],
                    &fi.idxVertex[2], &fi.idxTex[2], &fi.idxNormal[2]);

                if (i < (3 * 3) + 1) continue;

            } else {
                fprintf(stderr, "error: unsupported face configuration (%s)\n", l);
                continue;
            }

            current_vert->indices.push_back(fi);
            continue;
        }
    }
    delete[] line;

    // File parsing ended. We can close the file
    fclose(fObj);

    // TODO: Read the mtl file, just to put a light object on each  emitter?
    //       or maybe emission can be a property of the material?

    auto mesh_ret = std::vector<Mesh*>{};

    // Convert those vertex groups in meshes and vertex data objects
    // Ensure that every index references an unique combination of vertex, normal and texcoords
    for (const auto& vg : verts) {
        if (!vg.hasNormal && !vg.hasTexture) continue;  // No normal and no texture? Unsupported
        log->write(
            "meshopener::obj", LogType::Debug, "mesh %s, %zu vertex lists, normals=%s, textures=%s",
            vg.name.c_str(), vg.vertices.size(), (vg.hasNormal ? "true" : "false"),
            (vg.hasTexture ? "true" : "false"));

        VertexDataGroup vdlist;
        std::vector<VertexInfo> vinfo;

        unsigned idx = 0;
        for (const auto& vl : vg.vertices) {
            if (vl.indices.size() == 0) continue;  // Remove null vertex lists

            std::vector<UniqueVertex> uvs;  // The unique combinations of n+v+t
            std::vector<unsigned int> index_list;

            uvs.reserve(vl.indices.size());
            index_list.reserve(vl.indices.size());

            unsigned uvidx = 0;
            log->write(
                "meshopener::obj", LogType::Debug, "\tvertex list %u, %zu edges", idx,
                vl.indices.size());

            /* The obj file creates a index unique for each normal, vertex or texcoords.
             *
             * The videocard reads a index unique for the three component
             * This means that, if one of the three is different, it's a different vertex to the
             * videocard
             *
             * We need to transforme the index unique to each parameter to the index
             * unique for the combination of the three
             */
            for (const auto& idx : vl.indices) {
                for (auto fi = 0; fi < 3; fi++) {
                    UniqueVertex iuv;
                    iuv.idxVertex   = idx.idxVertex[fi] - 1;
                    iuv.idxNormal   = idx.idxNormal[fi] - 1;
                    iuv.idxTexcoord = idx.idxTex[fi] - 1;

                    auto founduv = std::find_if(
                        uvs.begin(), uvs.end(),
                        [&iuv](const UniqueVertex& fuv) { return iuv == fuv; });
                    if (founduv == std::end(uvs)) {
                        iuv.idx = uvidx++;
                        index_list.push_back(iuv.idx);

                        uvs.push_back(std::move(iuv));
                    } else {
                        index_list.push_back(founduv->idx);
                    }
                }
            }

            log->write(
                "meshopener::obj", LogType::Debug,
                "%zu unique vert/texcoord/normal combinations detected, %zu indices\n", uvs.size(),
                index_list.size());

            // Build the vertex data
            VertexData vdata;
            Material* mtl = nullptr;

            // Make groups of 3 vertices, so the vga knows this is a triangle
            // TODO: Support vertex indices, and modify this loop
            for (const auto& idxitem : index_list) {
                auto uv = uvs[idxitem];

                const auto uvv =
                    (uv.idxVertex < 0) ? (vertices.size() + uv.idxVertex) : uv.idxVertex;
                const auto uvn =
                    (uv.idxNormal < 0) ? (normals.size() + uv.idxNormal) : uv.idxNormal;
                const auto uvt =
                    (uv.idxTexcoord < 0) ? (texcoords.size() + uv.idxTexcoord) : uv.idxTexcoord;

                vdata.position.push_back(vertices[uvv]);

                if (vg.hasNormal) vdata.normals.push_back(normals[uvn]);

                if (vg.hasTexture) vdata.texcoords.push_back(texcoords[uvt]);

                if (vl.mtlname != "" && !mtl) {
                    mtl = GFXService::getMaterialManager()->getMaterial(vl.mtlname.c_str());
                }
            }

            auto fshader = GFXService::getShaderManager()->getShader("forward");

            if (mtl) {
                VertexInfo vi(idx, mtl->getID(), fshader, VertexRenderStyle::Triangles);
                vi.hasTexCoords = vg.hasTexture;
                vinfo.push_back(vi);
                mtl = nullptr;

            } else {
                log->write(
                    "meshopener::obj", LogType::Warning, "cannot load material %s for %s",
                    vl.mtlname.c_str(), vg.name.c_str());

                VertexInfo vi(idx, -1, fshader, VertexRenderStyle::Triangles);
                vi.hasTexCoords = vg.hasTexture;
                vinfo.push_back(vi);

                log->write(
                    "meshopener::obj", LogType::Warning, "\ta default material is being used");
            }

            vdlist.push_back(std::move(vdata));
            idx++;
        }

        auto mesh = new Mesh{vg.name.c_str(), new StaticAnimator{vdlist}, vinfo};
        mesh_ret.push_back(mesh);
    }

    fflush(stdout);

    return mesh_ret;
}
