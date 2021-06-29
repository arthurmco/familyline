#include <gtest/gtest.h>

#include <client/graphical/meshopener/MD2Opener.hpp>
#include <client/graphical/meshopener/OBJOpener.hpp>
#include <client/graphical/scene_manager.hpp>
#include <common/logic/logic_service.hpp>
#include <common/logic/object_factory.hpp>
#include <memory>
#include <unordered_map>

#include "utils.hpp"
#include "utils/test_renderer.hpp"
#include "utils/test_shader.hpp"

using namespace familyline;
using namespace familyline::graphics;

TEST(SceneManager, TestMeshAdd)
{
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    TestRenderer renderer;
    Camera camera(glm::vec3(-30, 30, -30), 16 / 9.0f, glm::vec3(0, 0, 0));
    SceneManager sm(renderer, camera);

    OBJOpener o;
    std::vector<Mesh*> meshes = o.OpenSpecialized(TESTS_DIR "/assets/test2.obj");
    Mesh* m                   = meshes[0];

    ASSERT_EQ(0, renderer.getVertexListCount());
    sm.add(make_scene_object(*m));
    ASSERT_EQ(3, renderer.getVertexListCount());

    sm.update(16);
    ASSERT_EQ(3, renderer.getVertexListCount());

    for (auto* m : meshes) delete m;
}

TEST(SceneManager, Test2MeshAdd)
{
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    TestRenderer renderer;
    Camera camera(glm::vec3(-30, 30, -30), 16 / 9.0f, glm::vec3(0, 0, 0));
    SceneManager sm(renderer, camera);

    OBJOpener o;
    std::vector<Mesh*> meshes  = o.OpenSpecialized(TESTS_DIR "/assets/test.obj");
    Mesh* m1                   = meshes[0];
    std::vector<Mesh*> meshes2 = o.OpenSpecialized(TESTS_DIR "/assets/test2.obj");
    Mesh* m2                   = meshes2[0];

    ASSERT_EQ(0, renderer.getVertexListCount());
    sm.add(make_scene_object(*m1));
    sm.add(make_scene_object(*m2));
    ASSERT_EQ(4, renderer.getVertexListCount());

    sm.update(16);
    ASSERT_EQ(4, renderer.getVertexListCount());

    for (auto* m : meshes) delete m;

    for (auto* m : meshes2) delete m;
}

TEST(SceneManager, TestMeshPlot)
{
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    TestRenderer renderer;
    Camera camera(glm::vec3(-30, 30, -30), 16 / 9.0f, glm::vec3(0, 0, 0));
    SceneManager sm(renderer, camera);

    OBJOpener o;
    std::vector<Mesh*> meshes = o.OpenSpecialized(TESTS_DIR "/assets/test2.obj");
    Mesh* m                   = meshes[0];
    m->setLogicPosition(glm::vec3(10, 0, 15));

    sm.add(make_scene_object(*m));
    sm.update(16);

    ASSERT_FLOAT_EQ(10, m->getPosition().x);
    ASSERT_FLOAT_EQ(0, m->getPosition().y);
    ASSERT_FLOAT_EQ(15, m->getPosition().z);

    for (auto* m : meshes) delete m;
}

TEST(SceneManager, TestMeshRemove)
{
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    TestRenderer renderer;
    Camera camera(glm::vec3(-30, 30, -30), 16 / 9.0f, glm::vec3(0, 0, 0));
    SceneManager sm(renderer, camera);

    OBJOpener o;
    std::vector<Mesh*> meshes  = o.OpenSpecialized(TESTS_DIR "/assets/test.obj");
    std::vector<Mesh*> meshes2 = o.OpenSpecialized(TESTS_DIR "/assets/test2.obj");
    std::vector<Mesh*> meshes3 = o.OpenSpecialized(TESTS_DIR "/assets/test2.obj");
    std::vector<Mesh*> meshes4 = o.OpenSpecialized(TESTS_DIR "/assets/test.obj");

    sm.add(make_scene_object(*meshes[0]));
    sm.add(make_scene_object(*meshes2[0]));
    auto removeid = sm.add(make_scene_object(*meshes3[0]));
    sm.add(make_scene_object(*meshes4[0]));
    sm.update(16);

    ASSERT_EQ(8, renderer.getVertexListCount());

    sm.remove(removeid);
    sm.update(16);

    ASSERT_EQ(5, renderer.getVertexListCount());

    for (auto* m : meshes) delete m;

    for (auto* m : meshes2) delete m;

    for (auto* m : meshes3) delete m;

    for (auto* m : meshes4) delete m;
}

TEST(SceneManager, TestSceneManagerAnimate)
{
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    TestRenderer renderer;
    Camera camera(glm::vec3(-30, 30, -30), 16 / 9.0f, glm::vec3(0, 0, 0));
    SceneManager sm(renderer, camera);

    OBJOpener oo;
    MD2Opener om;
    std::vector<Mesh*> meshes  = oo.OpenSpecialized(TESTS_DIR "/assets/test.obj");
    std::vector<Mesh*> meshes2 = om.OpenSpecialized(TESTS_DIR "/assets/anim_test.md2");
    Mesh* animmesh             = meshes2[0];

    sm.add(make_scene_object(*meshes[0]));
    sm.add(make_scene_object(*animmesh));

    ASSERT_FLOAT_EQ(0.0, animmesh->getAnimator()->getCurrentTime());

    for (auto i = 0; i < 120; i++) {
        sm.update(15);
    }

    ASSERT_FLOAT_EQ(1800.0, animmesh->getAnimator()->getCurrentTime());

    for (auto* m : meshes) delete m;

    for (auto* m : meshes2) delete m;
}

TEST(SceneManager, TestSceneManagerAnimateTwo)
{
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    TestRenderer renderer;
    Camera camera(glm::vec3(-30, 30, -30), 16 / 9.0f, glm::vec3(0, 0, 0));
    SceneManager sm(renderer, camera);

    OBJOpener oo;
    MD2Opener om;
    std::vector<Mesh*> meshes  = oo.OpenSpecialized(TESTS_DIR "/assets/test.obj");
    std::vector<Mesh*> meshes2 = om.OpenSpecialized(TESTS_DIR "/assets/anim_test.md2");
    std::vector<Mesh*> meshes3 = om.OpenSpecialized(TESTS_DIR "/assets/anim_test.md2");
    Mesh* animmesh             = meshes2[0];
    Mesh* animmesh2            = meshes3[0];

    sm.add(make_scene_object(*meshes[0]));
    sm.add(make_scene_object(*animmesh));

    ASSERT_FLOAT_EQ(0.0, animmesh->getAnimator()->getCurrentTime());

    for (auto i = 0; i < 20; i++) {
        sm.update(15);
    }

    ASSERT_FLOAT_EQ(300.0, animmesh->getAnimator()->getCurrentTime());
    sm.add(make_scene_object(*animmesh2));
    ASSERT_FLOAT_EQ(0.0, animmesh2->getAnimator()->getCurrentTime());

    for (auto i = 0; i < 30; i++) {
        sm.update(15);
    }

    ASSERT_FLOAT_EQ(750.0, animmesh->getAnimator()->getCurrentTime());
    ASSERT_FLOAT_EQ(450.0, animmesh2->getAnimator()->getCurrentTime());

    for (auto* m : meshes) delete m;

    for (auto* m : meshes2) delete m;

    for (auto* m : meshes3) delete m;
}
