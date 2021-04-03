#include <gtest/gtest.h>

#include <client/graphical/mesh.hpp>
#include <client/graphical/gfx_service.hpp>
#include <client/graphical/meshopener/OBJOpener.hpp>
#include <client/graphical/meshopener/MD2Opener.hpp>
#include "utils/test_shader.hpp"

using namespace familyline::graphics;


TEST(ModelOpener, TestIfOBJOpens) {
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    OBJOpener o;
    std::vector<Mesh*> meshes = o.OpenSpecialized(TESTS_DIR "/assets/test2.obj");

    ASSERT_EQ(1, meshes.size());
    ASSERT_STREQ("testcube", meshes[0]->getName().data());

    auto vg0 = meshes[0]->getAnimator()->getCurrentFrame();
    meshes[0]->getAnimator()->advance(1000);
    auto vg1 = meshes[0]->getAnimator()->getCurrentFrame();

    ASSERT_EQ(3, vg0.size());
    ASSERT_EQ(3, vg1.size());

    glm::vec3 psum[2] = {glm::vec3(0), glm::vec3(0)};
    for (auto& p : vg0[0].position) {
        psum[0] += p;
    }

    for (auto& p : vg1[0].position) {
        psum[1] += p;
    }

    ASSERT_EQ(psum[0], psum[1]);

    auto bb = meshes[0]->getBoundingBox();
    EXPECT_FLOAT_EQ(-1.665732, bb.minX);
    EXPECT_FLOAT_EQ(0.208672, bb.minY);
    EXPECT_FLOAT_EQ(-1.665732, bb.minZ);
    EXPECT_FLOAT_EQ(1.943095, bb.maxX);
    EXPECT_FLOAT_EQ(6.6151862, bb.maxY);
    EXPECT_FLOAT_EQ(1.665732, bb.maxZ);
    
    GFXService::getShaderManager()->clear();
}


TEST(ModelOpener, TestIfStaticMD2Opens) {
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    MD2Opener o;
    std::vector<Mesh*> meshes = o.OpenSpecialized(TESTS_DIR "/assets/test.md2");

    ASSERT_EQ(1, meshes.size());
    ASSERT_STREQ("test_name", meshes[0]->getName().data());

    auto vg0 = meshes[0]->getAnimator()->getCurrentFrame();
    meshes[0]->getAnimator()->advance(1000);
    auto vg1 = meshes[0]->getAnimator()->getCurrentFrame();

    ASSERT_EQ(1, vg0.size());
    ASSERT_EQ(1, vg1.size());

    glm::vec3 psum[2] = {glm::vec3(0), glm::vec3(0)};
    for (auto& p : vg0[0].position) {
        psum[0] += p;
    }

    for (auto& p : vg1[0].position) {
        psum[1] += p;
    }

    ASSERT_EQ(psum[0], psum[1]);
    
    auto bb = meshes[0]->getBoundingBox();
    EXPECT_FLOAT_EQ(-1.0, bb.minX);
    EXPECT_FLOAT_EQ(-1.0, bb.minY);
    EXPECT_FLOAT_EQ(-1.0, bb.minZ);
    EXPECT_FLOAT_EQ(1.0, bb.maxX);
    EXPECT_FLOAT_EQ(1.0, bb.maxY);
    EXPECT_FLOAT_EQ(1.0, bb.maxZ);
        
    GFXService::getShaderManager()->clear();
}

TEST(ModelOpener, TestIfDynamicMD2Opens) {
    TestShaderProgram s{"forward"};
    GFXService::getShaderManager()->addShader(&s);

    MD2Opener o;
    std::vector<Mesh*> meshes = o.OpenSpecialized(TESTS_DIR "/assets/anim_test.md2");

    ASSERT_EQ(1, meshes.size());
    ASSERT_STREQ("test_name", meshes[0]->getName().data());

    auto vg0 = meshes[0]->getAnimator()->getCurrentFrame();
    meshes[0]->getAnimator()->advance(1000);
    auto vg1 = meshes[0]->getAnimator()->getCurrentFrame();

    ASSERT_EQ(1, vg0.size());
    ASSERT_EQ(1, vg1.size());

    glm::vec3 psum[2] = {glm::vec3(0), glm::vec3(0)};
    for (auto& p : vg0[0].position) {
        psum[0] += p;
    }

    for (auto& p : vg1[0].position) {
        psum[1] += p;
    }

    ASSERT_NE(psum[0], psum[1]);
    
    auto bb = meshes[0]->getBoundingBox();
    EXPECT_FLOAT_EQ(-1.3170778, bb.minX);
    EXPECT_FLOAT_EQ(-0.018443191, bb.minY);
    EXPECT_FLOAT_EQ(-1.3170779, bb.minZ);
    EXPECT_FLOAT_EQ(1.3170782, bb.maxX);
    EXPECT_FLOAT_EQ(4.4252915, bb.maxY);
    EXPECT_FLOAT_EQ(1.3170781, bb.maxZ);

    for (auto* m: meshes)
        delete m;
    
    GFXService::getShaderManager()->clear();
}
