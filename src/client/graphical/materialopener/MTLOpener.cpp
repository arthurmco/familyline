#include <client/graphical/materialopener/../gfx_service.hpp>
#include <client/graphical/materialopener/../texture_manager.hpp>
#include <client/graphical/materialopener/MTLOpener.hpp>
#include <common/logger.hpp>

using namespace familyline::graphics;

std::vector<Material*> MTLOpener::Open(const char* file)
{
    auto& log  = LoggerService::getLogger();
    FILE* fMat = fopen(file, "r");

    if (!fMat) {
        char s[512];
        snprintf(s, 511, "Failure to open material %s (error %d)", file, errno);
        throw asset_exception(s, AssetError::AssetOpenError);
    }

    std::vector<Material*> mats;
    char* matname = nullptr;
    glm::vec3 diffuse, ambient, specular;

    char line[256];
    while (!feof(fMat)) {
        char* fline = &line[0];
        memset(fline, 0, 255);
        fgets(fline, 255, fMat);

        int cind = 0;
        while (isspace(fline[cind])) {
            cind++;
        }

        if (cind > 255) {
            continue;  // avoid buffer overflows.
        }

        fline = &fline[cind];

        if (fline[0] == '#') continue;  // Discard comment

        if (!strncmp(fline, "newmtl", 6)) {
            /* New material declared */
            if (matname) {
                /* Add the last processed material and discard it */
                MaterialData md;
                md.diffuseColor  = diffuse;
                md.specularColor = specular;
                md.ambientColor  = diffuse * 0.1f;
                Material* m      = new Material{(int)mats.size(), matname, md};
                mats.push_back(m);
            }

            if (matname)
                delete[] matname;
            
            matname = new char[256];
            memset((void*)matname, 0, 255);

            int i = 7;
            while (fline[i] != '\n') {
                matname[i - 7] = fline[i];
                i++;
                if (i > 256) break;
            }

            continue;
        }

        if (!strncmp(fline, "Ka", 2)) {
            fline += 2;
            sscanf(fline, "%f %f %f", &ambient.x, &ambient.y, &ambient.z);
            continue;
        }
        if (!strncmp(fline, "Kd", 2)) {
            fline += 2;
            sscanf(fline, "%f %f %f", &diffuse.x, &diffuse.y, &diffuse.z);
            continue;
        }
        if (!strncmp(fline, "Ks", 2)) {
            fline += 2;
            sscanf(fline, "%f %f %f", &specular.x, &specular.y, &specular.z);
            continue;
        }

        if (!strncmp(fline, "map_Kd", 6)) {
            /* Texture */
            fline += 6;
            char texpath[256];
            sscanf(fline, "%s\n", texpath);

            std::string texname{texpath};
            texname[texname.find_last_of('.')] = 0;

            Texture* t = TextureOpener::OpenTexture(texpath);

            if (t) {
                GFXService::getTextureManager()->AddTexture(texname.c_str(), t);
            } else {
                log->write(
                    "material-opener::mtl", LogType::Warning, "Texture %s failed to load", texpath);
            }

            continue;
        }
    }

    /* Add the last material */
    MaterialData md;
    md.diffuseColor  = diffuse;
    md.specularColor = specular;
    md.ambientColor  = diffuse * 0.1f;
    Material* m      = new Material{(int)mats.size(), matname, md};
    mats.push_back(m);

    if (matname)
        delete[] matname;

    fclose(fMat);
    return mats;
}
