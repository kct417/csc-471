/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:
    WindowManager *windowManager = nullptr;

    // Our shader program - use this one for Blinn-Phong
    std::shared_ptr<Program> prog;

    // Our shader program for textures
    std::shared_ptr<Program> texProg;

    // shadow shader program
    std::shared_ptr<Program> shadowProg;

    // our geometry
    shared_ptr<Shape> sphere;

    vector<shared_ptr<Shape>> eeveeMeshes;
    vector<shared_ptr<Shape>> dittoMeshes;
    vector<shared_ptr<Shape>> snorlaxMeshes;
    vector<shared_ptr<Shape>> bulbasaurMeshes;

    vector<shared_ptr<pair<vec3, vec3>>> eeveeMinsMaxs;
    vector<shared_ptr<pair<vec3, vec3>>> dittoMinsMaxs;
    vector<shared_ptr<pair<vec3, vec3>>> snorlaxMinsMaxs;
    vector<shared_ptr<pair<vec3, vec3>>> bulbasaurMinsMaxs;

    shared_ptr<Shape> noNormals;

    // global data for ground plane - direct load constant defined CPU data to GPU (not obj)
    GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
    int g_GiboLen;
    // ground VAO
    GLuint GroundVertexArrayID;

    // the image to use as a texture (ground)
    shared_ptr<Texture> texture0;

    // ground data
    float g_groundSize;
    float g_groundY;

    // global data (larger program should be encapsulated)
    vec3 gMin;
    float gRot = 0;
    float gCamH = 0;
    // animation data
    float lightTrans = 0;
    float gTrans = -3;
    float sTheta = 0;
    float eTheta = 0;
    float hTheta = 0;

    // camera data
    glm::vec3 eye = glm::vec3(0.0f, 0.0f, 0.0f);
    float radius = 1.0f;
    float phi = 0.0f;
    float theta = glm::pi<float>() / 2.0f;
    float sensitivity = 0.05f;

    // toggle for animation
    bool toggleMaterial = false;
    bool toggleAnimate = false;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        // update global camera rotate
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            gRot -= 0.2;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            gRot += 0.2;
        }
        // update camera height
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
        {
            gCamH += 0.25;
        }
        if (key == GLFW_KEY_F && action == GLFW_PRESS)
        {
            gCamH -= 0.25;
        }

        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            lightTrans += 0.25;
        }
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            lightTrans -= 0.25;
        }
        if (key == GLFW_KEY_M && action == GLFW_PRESS)
        {
            toggleMaterial = !toggleMaterial;
        }
        if (key == GLFW_KEY_X && action == GLFW_PRESS)
        {
            toggleAnimate = !toggleAnimate;
        }
        if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    void mouseCallback(GLFWwindow *window, int button, int action, int mods)
    {
        double posX, posY;

        if (action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &posX, &posY);
            cout << "Pos X " << posX << " Pos Y " << posY << endl;
        }
    }

    void scrollCallback(GLFWwindow *window, double deltaX, double deltaY)
    {
        theta += deltaX * sensitivity;
        phi += deltaY * sensitivity;

        float limit = glm::radians(89.0f);
        phi = glm::clamp(phi, -limit, limit);
    }

    void resizeCallback(GLFWwindow *window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void init(const std::string &resourceDirectory)
    {
        GLSL::checkVersion();

        // Set background color.
        glClearColor(.72f, .84f, 1.06f, 1.0f);
        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);

        // Initialize the GLSL program that we will use for local shading
        prog = make_shared<Program>();
        prog->setVerbose(true);
        prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
        prog->init();
        prog->addUniform("P");
        prog->addUniform("V");
        prog->addUniform("M");
        prog->addUniform("MatAmb");
        prog->addUniform("MatDif");
        prog->addUniform("MatSpec");
        prog->addUniform("MatShine");
        prog->addUniform("lightPos");
        prog->addUniform("lightColor");
        prog->addAttribute("vertPos");
        prog->addAttribute("vertNor");

        // Initialize the GLSL program that we will use for texture mapping
        texProg = make_shared<Program>();
        texProg->setVerbose(true);
        texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
        texProg->init();
        texProg->addUniform("P");
        texProg->addUniform("V");
        texProg->addUniform("M");
        texProg->addUniform("Texture0");
        texProg->addAttribute("vertPos");
        texProg->addAttribute("vertNor");
        texProg->addAttribute("vertTex");

        // read in a load the texture
        texture0 = make_shared<Texture>();
        texture0->setFilename(resourceDirectory + "/grass.jpg");
        texture0->init();
        texture0->setUnit(0);
        texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        // Initialize the GLSL program that we will use for shadows
        shadowProg = make_shared<Program>();
        shadowProg->setVerbose(true);
        shadowProg->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/shadow_frag.glsl");
        shadowProg->init();
        shadowProg->addUniform("P");
        shadowProg->addUniform("V");
        shadowProg->addUniform("M");
        shadowProg->addUniform("lightPos");
        shadowProg->addAttribute("vertPos");
        shadowProg->addAttribute("vertNor");
    }

    void initGeom(const std::string &resourceDirectory)
    {
        // EXAMPLE set up to read one shape from one obj file - convert to read several
        //  Initialize mesh
        //  Load geometry
        //  Some obj files contain material information.We'll ignore them for this assignment.
        vector<tinyobj::shape_t> TOshapes;
        vector<tinyobj::material_t> objMaterials;
        string errStr;
        // load in the mesh and make the shape(s)
        bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/SmoothSphere.obj").c_str());
        if (!rc)
        {
            cerr << errStr << endl;
        }
        else
        {
            sphere = make_shared<Shape>();
            sphere->createShape(TOshapes[0]);
            sphere->measure();
            sphere->init();
        }
        // read out information stored in the shape about its size - something like this...
        // then do something with that information.....
        gMin.x = sphere->min.x;
        gMin.y = sphere->min.y;

        auto loadMultiShape = [&](const std::string &filename) -> std::pair<vector<shared_ptr<Shape>>, vector<shared_ptr<pair<vec3, vec3>>>>
        {
            vector<shared_ptr<Shape>> shapesVec;
            vector<shared_ptr<pair<vec3, vec3>>> minMaxVec;

            vector<tinyobj::shape_t> shapes;
            vector<tinyobj::material_t> materials;
            string errStr;

            bool rc = tinyobj::LoadObj(shapes, materials, errStr, (resourceDirectory + "/" + filename).c_str());
            if (!rc)
            {
                cerr << "Failed to load " << filename << ": " << errStr << endl;
                return {shapesVec, minMaxVec};
            }

            for (auto &shape : shapes)
            {
                auto mesh = make_shared<Shape>();
                mesh->createShape(shape);
                mesh->measure();
                mesh->init();

                shapesVec.push_back(mesh);

                // Store min and max of this submesh
                auto minMaxPair = make_shared<pair<vec3, vec3>>(mesh->min, mesh->max);
                minMaxVec.push_back(minMaxPair);
            }

            return {shapesVec, minMaxVec};
        };

        auto eeveeData = loadMultiShape("eevee.obj");
        eeveeMeshes = eeveeData.first;
        eeveeMinsMaxs = eeveeData.second;

        auto dittoData = loadMultiShape("ditto.obj");
        dittoMeshes = dittoData.first;
        dittoMinsMaxs = dittoData.second;

        auto snorlaxData = loadMultiShape("snorlax.obj");
        snorlaxMeshes = snorlaxData.first;
        snorlaxMinsMaxs = snorlaxData.second;

        auto bulbasaurData = loadMultiShape("bulbasaur.obj");
        bulbasaurMeshes = bulbasaurData.first;
        bulbasaurMinsMaxs = bulbasaurData.second;

        vector<tinyobj::shape_t> TOshapesC;
        vector<tinyobj::material_t> objMaterialsC;
        rc = tinyobj::LoadObj(TOshapesC, objMaterialsC, errStr, (resourceDirectory + "/icoNoNormals.obj").c_str());
        if (!rc)
        {
            cerr << errStr << endl;
        }
        else
        {
            noNormals = make_shared<Shape>();
            noNormals->createShape(TOshapesC[0]);
            noNormals->measure();
            noNormals->init();
        }

        // code to load in the ground plane (CPU defined data passed to GPU)
        initGround();
    }

    // directly pass quad for the ground to the GPU
    void initGround()
    {

        g_groundSize = 20;
        g_groundY = -0.25;

        // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
        float GrndPos[] = {
            -g_groundSize, g_groundY, -g_groundSize,
            -g_groundSize, g_groundY, g_groundSize,
            g_groundSize, g_groundY, g_groundSize,
            g_groundSize, g_groundY, -g_groundSize};

        float GrndNorm[] = {
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0};

        static GLfloat GrndTex[] = {
            0, 0, // back
            0, 1,
            1, 1,
            1, 0};

        unsigned short idx[] = {0, 1, 2, 0, 2, 3};

        // generate the ground VAO
        glGenVertexArrays(1, &GroundVertexArrayID);
        glBindVertexArray(GroundVertexArrayID);

        g_GiboLen = 6;
        glGenBuffers(1, &GrndBuffObj);
        glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

        glGenBuffers(1, &GrndNorBuffObj);
        glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

        glGenBuffers(1, &GrndTexBuffObj);
        glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

        glGenBuffers(1, &GIndxBuffObj);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    }

    // code to draw the ground plane
    void drawGround(shared_ptr<Program> curS)
    {
        curS->bind();
        glBindVertexArray(GroundVertexArrayID);
        texture0->bind(curS->getUniform("Texture0"));
        // draw the ground plane
        SetModel(vec3(0, -1, 0), 0, 0, 1, curS);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // draw!
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
        glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        curS->unbind();
    }

    // helper function to pass material data to the GPU
    void SetMaterial(shared_ptr<Program> curS, int i)
    {
        switch (i)
        {
        // eevee
        case 0:
            // shiny eevee
            if (toggleMaterial)
            {
                glUniform3f(curS->getUniform("MatAmb"), 0.03f, 0.025f, 0.015f);
                glUniform3f(curS->getUniform("MatDif"), 0.9f, 0.8f, 0.55f);
                glUniform3f(curS->getUniform("MatSpec"), 0.25f, 0.22f, 0.18f);
                glUniform1f(curS->getUniform("MatShine"), 28.0f);
            }
            // default eevee
            else
            {
                glUniform3f(curS->getUniform("MatAmb"), 0.03f, 0.02f, 0.01f);
                glUniform3f(curS->getUniform("MatDif"), 0.55f, 0.33f, 0.2f);
                glUniform3f(curS->getUniform("MatSpec"), 0.2f, 0.18f, 0.15f);
                glUniform1f(curS->getUniform("MatShine"), 20.0f);
            }
            break;
        // ditto
        case 1:
            glUniform3f(curS->getUniform("MatAmb"), 0.03f, 0.02f, 0.04f);
            glUniform3f(curS->getUniform("MatDif"), 0.8f, 0.6f, 0.9f);
            glUniform3f(curS->getUniform("MatSpec"), 0.25f, 0.2f, 0.3f);
            glUniform1f(curS->getUniform("MatShine"), 20.0f);
            break;
        // snorlax
        case 2:
            glUniform3f(curS->getUniform("MatAmb"), 0.02f, 0.02f, 0.03f);
            glUniform3f(curS->getUniform("MatDif"), 0.15f, 0.3f, 0.45f);
            glUniform3f(curS->getUniform("MatSpec"), 0.2f, 0.25f, 0.3f);
            glUniform1f(curS->getUniform("MatShine"), 16.0f);
            break;
        // bulbasaur
        case 3:
            glUniform3f(curS->getUniform("MatAmb"), 0.02f, 0.03f, 0.02f);
            glUniform3f(curS->getUniform("MatDif"), 0.2f, 0.6f, 0.4f);
            glUniform3f(curS->getUniform("MatSpec"), 0.15f, 0.2f, 0.15f);
            glUniform1f(curS->getUniform("MatShine"), 20.0f);
            break;
        // hierarchical model
        case 4:
            glUniform3f(curS->getUniform("MatAmb"), 0.25f, 0.20f, 0.17f);
            glUniform3f(curS->getUniform("MatDif"), 0.76f, 0.60f, 0.50f);
            glUniform3f(curS->getUniform("MatSpec"), 0.3f, 0.3f, 0.3f);
            glUniform1f(curS->getUniform("MatShine"), 16.0f);
        }
    }

    void SetLight(shared_ptr<Program> curS)
    {
        glUniform3f(curS->getUniform("lightPos"), -2.0f + lightTrans, 2.0f, 2.0f);
        glUniform3f(curS->getUniform("lightColor"), 1.0f, 1.0f, 1.0f);
    }

    /* helper function to set model trasnforms */
    void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS)
    {
        mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
        mat4 RotX = glm::rotate(glm::mat4(1.0f), rotX, vec3(1, 0, 0));
        mat4 RotY = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
        mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
        mat4 ctm = Trans * RotX * RotY * ScaleS;
        glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
    }

    void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack> M)
    {
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    }

    /* code to draw waving hierarchical model */
    void drawHierModel(shared_ptr<MatrixStack> Model)
    {
        // draw mesh
        Model->pushMatrix();
        {
            Model->loadIdentity();
            Model->translate(vec3(0, 2.5, -10));
            /* draw top cube - aka head */
            Model->pushMatrix();
            {
                Model->translate(vec3(0, 1.4, 0));
                Model->scale(vec3(0.5, 0.5, 0.5));
                setModel(prog, Model);
                noNormals->draw(prog);
            }
            Model->popMatrix();
            // draw the torso with these transforms
            Model->pushMatrix();
            {
                Model->scale(vec3(1.15, 1.35, 1.25));
                setModel(prog, Model);
                noNormals->draw(prog);
            }
            Model->popMatrix();
            // draw the upper 'arm' - relative
            // note you must change this to include 3 components!
            Model->pushMatrix();
            { // place at shoulder
                Model->translate(vec3(0.8, 0.8, 0));
                // rotate shoulder joint
                if (toggleAnimate)
                    Model->rotate(sTheta / 4, vec3(0, 0, 1));
                Model->rotate(glm::radians(30.0f), vec3(0, 0, 1));
                // move to shoulder joint
                Model->translate(vec3(0.8, 0, 0));

                // now draw lower arm - this is INCOMPLETE and you will add a 3rd component
                // right now this is in the SAME place as the upper arm
                Model->pushMatrix();
                {
                    Model->translate(vec3(0.6, 0, 0));
                    if (toggleAnimate)
                        Model->rotate(sTheta / 4, vec3(0, 0, 1));
                    Model->rotate(glm::radians(30.0f), vec3(0, 0, 1));
                    Model->translate(vec3(0.6, 0, 0));

                    // hand
                    Model->pushMatrix();
                    {
                        Model->translate(vec3(0.5, 0, 0));
                        if (toggleAnimate)
                            Model->rotate(sTheta / 8, vec3(0, 0, 1));
                        Model->rotate(glm::radians(15.0f), vec3(0, 0, 1));
                        Model->translate(vec3(0.5, 0, 0));

                        // fingers
                        for (int i = 0; i < 4; i++)
                        {
                            Model->pushMatrix();
                            {
                                float angle = glm::radians(-40.0f + i * 17.5f);
                                Model->translate(vec3(0.05, 0.0, 0.0));
                                Model->rotate(angle, vec3(0, 0, 1));
                                if (toggleAnimate)
                                    Model->rotate(sTheta / 6, vec3(0, 0, 1));

                                Model->translate(vec3(0.25, 0, 0));
                                Model->scale(vec3(0.3, 0.06, 0.06));
                                setModel(prog, Model);
                                noNormals->draw(prog);
                            }
                            Model->popMatrix();
                        }

                        // thumb
                        Model->pushMatrix();
                        {
                            Model->translate(vec3(0.15, 0.0, 0.0));
                            Model->rotate(glm::radians(45.0f), vec3(0, 0, 1));
                            if (toggleAnimate)
                                Model->rotate(sTheta / 6, vec3(0, 0, 1));

                            Model->translate(vec3(0.1, 0.15, 0));
                            Model->scale(vec3(0.3, 0.06, 0.06));
                            setModel(prog, Model);
                            noNormals->draw(prog);
                        }
                        Model->popMatrix();

                        // non-uniform scale
                        Model->scale(vec3(0.4, 0.25, 0.15));
                        setModel(prog, Model);
                        noNormals->draw(prog);
                    }
                    Model->popMatrix();

                    Model->scale(vec3(0.8, 0.3, 0.3));
                    setModel(prog, Model);
                    noNormals->draw(prog);
                }
                Model->popMatrix();

                // Do final scale ONLY to upper arm then draw
                // non-uniform scale
                Model->scale(vec3(0.8, 0.25, 0.25));
                setModel(prog, Model);
                noNormals->draw(prog);
            }
            Model->popMatrix();

            // draw the upper 'arm' - relative
            // note you must change this to include 3 components!
            Model->pushMatrix();
            { // place at shoulder
                Model->translate(vec3(-0.9, 0.8, 0));
                // rotate shoulder joint
                Model->rotate(glm::radians(50.0f), vec3(0, 0, 1));
                Model->rotate(glm::radians(0.0f) + gRot, vec3(0, 1, 0));
                // move to shoulder joint
                Model->translate(vec3(-0.8, 0, 0));

                // now draw lower arm - this is INCOMPLETE and you will add a 3rd component
                // right now this is in the SAME place as the upper arm
                Model->pushMatrix();
                {
                    Model->translate(vec3(-0.6, 0, 0));
                    Model->rotate(glm::radians(25.0f), vec3(0, 0, 1));
                    Model->translate(vec3(-0.6, 0, 0));

                    // hand
                    Model->pushMatrix();
                    {
                        Model->translate(vec3(-0.5, 0, 0));
                        Model->rotate(glm::radians(15.0f), vec3(0, 0, 1));
                        Model->translate(vec3(-0.5, 0, 0));

                        // fingers
                        for (int i = 0; i < 4; i++)
                        {
                            Model->pushMatrix();
                            {
                                float angle = glm::radians(20.0f - i * 17.5f);
                                Model->translate(vec3(-0.05, 0, 0));
                                Model->rotate(angle, vec3(0, 0, 1));

                                Model->translate(vec3(-0.25, 0, 0));
                                Model->scale(vec3(0.3, 0.06, 0.06));
                                setModel(prog, Model);
                                noNormals->draw(prog);
                            }
                            Model->popMatrix();
                        }

                        // thumb
                        Model->pushMatrix();
                        {
                            Model->translate(vec3(-0.15, 0.0, 0.0));
                            Model->rotate(glm::radians(45.0f), vec3(0, 0, 1));

                            Model->translate(vec3(-0.1, -0.15, 0));
                            Model->scale(vec3(0.3, 0.06, 0.06));
                            setModel(prog, Model);
                            noNormals->draw(prog);
                        }
                        Model->popMatrix();

                        // non-uniform scale
                        Model->scale(vec3(-0.4, 0.25, 0.15));
                        setModel(prog, Model);
                        noNormals->draw(prog);
                    }
                    Model->popMatrix();

                    Model->scale(vec3(-0.8, 0.3, 0.3));
                    setModel(prog, Model);
                    noNormals->draw(prog);
                }
                Model->popMatrix();

                // Do final scale ONLY to upper arm then draw
                // non-uniform scale
                Model->scale(vec3(-0.8, 0.25, 0.25));
                setModel(prog, Model);
                noNormals->draw(prog);
            }
            Model->popMatrix();

            // leg
            Model->pushMatrix();
            {
                Model->translate(vec3(0.5, -1.25, 0));
                if (toggleAnimate)
                    Model->rotate(sTheta, vec3(1, 0, 0));
                Model->rotate(glm::radians(2.0f), vec3(0, 0, 1));
                Model->translate(vec3(0.1, -0.75, 0));

                // foot
                Model->pushMatrix();
                {
                    Model->translate(vec3(0, -1.35, 0.2));

                    // non-uniform scale
                    Model->scale(vec3(0.3, 0.2, 0.6));
                    setModel(prog, Model);
                    noNormals->draw(prog);
                }
                Model->popMatrix();

                // non-uniform scale
                Model->scale(vec3(0.45, 1.5, 0.8));
                setModel(prog, Model);
                noNormals->draw(prog);
            }
            Model->popMatrix();

            // other leg
            Model->pushMatrix();
            {
                Model->translate(vec3(-0.5, -1.25, 0));
                if (toggleAnimate)
                    Model->rotate(-sTheta, vec3(1, 0, 0));
                Model->rotate(-glm::radians(2.0f), vec3(0, 0, 1));
                Model->translate(vec3(-0.1, -0.75, 0));

                // foot
                Model->pushMatrix();
                {
                    Model->translate(vec3(0, -1.35, 0.2));

                    // non-uniform scale
                    Model->scale(vec3(0.3, 0.2, 0.6));
                    setModel(prog, Model);
                    noNormals->draw(prog);
                }
                Model->popMatrix();

                // non-uniform scale
                Model->scale(vec3(0.45, 1.5, 0.8));
                setModel(prog, Model);
                noNormals->draw(prog);
            }
            Model->popMatrix();
        }
        Model->popMatrix();
    }

    void drawShadow(shared_ptr<MatrixStack> Model, shared_ptr<Program> shadowProg, shared_ptr<Shape> shape, shared_ptr<MatrixStack> Projection, shared_ptr<MatrixStack> View)
    {
        shadowProg->bind();

        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        glUniformMatrix4fv(shadowProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));

        Model->pushMatrix();
        {
            const float epsilon = 0.001f;
            Model->translate(vec3(0.0f, g_groundY + epsilon, 0.0f));
            Model->scale(vec3(1.0f, 0.0f, 1.0f));
            setModel(shadowProg, Model);
            glDisable(GL_DEPTH_TEST);
            shape->draw(shadowProg);
            glEnable(GL_DEPTH_TEST);
        }
        Model->popMatrix();

        shadowProg->unbind();
        prog->bind();
    }

    void drawHierModelShadow(shared_ptr<MatrixStack> Model)
    {
        shadowProg->bind();

        Model->pushMatrix();
        {
            Model->loadIdentity();
            const float epsilon = 0.001f;
            Model->translate(vec3(0.0f, g_groundY + epsilon - 1.0f, -10.0f));
            Model->scale(vec3(1.0f, 0.0f, 1.0f));

            // Head
            Model->pushMatrix();
            {
                Model->translate(vec3(0, 1.4, 0));
                Model->scale(vec3(0.5, 0.5, 0.5));
                setModel(shadowProg, Model);
                noNormals->draw(shadowProg);
            }
            Model->popMatrix();

            // Torso
            Model->pushMatrix();
            {
                Model->scale(vec3(1.15, 1.35, 1.25));
                setModel(shadowProg, Model);
                noNormals->draw(shadowProg);
            }
            Model->popMatrix();

            // Right arm
            Model->pushMatrix();
            {
                Model->translate(vec3(0.8, 0.8, 0));
                if (toggleAnimate)
                    Model->rotate(sTheta / 4, vec3(0, 0, 1));
                Model->rotate(glm::radians(30.0f), vec3(0, 0, 1));
                Model->translate(vec3(0.8, 0, 0));

                // Lower arm
                Model->pushMatrix();
                {
                    Model->translate(vec3(0.6, 0, 0));
                    if (toggleAnimate)
                        Model->rotate(sTheta / 4, vec3(0, 0, 1));
                    Model->rotate(glm::radians(30.0f), vec3(0, 0, 1));
                    Model->translate(vec3(0.6, 0, 0));

                    // Hand
                    Model->pushMatrix();
                    {
                        Model->translate(vec3(0.5, 0, 0));
                        if (toggleAnimate)
                            Model->rotate(sTheta / 8, vec3(0, 0, 1));
                        Model->rotate(glm::radians(15.0f), vec3(0, 0, 1));
                        Model->translate(vec3(0.5, 0, 0));

                        // Fingers
                        for (int i = 0; i < 4; i++)
                        {
                            Model->pushMatrix();
                            {
                                float angle = glm::radians(-40.0f + i * 17.5f);
                                Model->translate(vec3(0.05, 0.0, 0.0));
                                Model->rotate(angle, vec3(0, 0, 1));
                                if (toggleAnimate)
                                    Model->rotate(sTheta / 6, vec3(0, 0, 1));

                                Model->translate(vec3(0.25, 0, 0));
                                Model->scale(vec3(0.3, 0.06, 0.06));
                                setModel(shadowProg, Model);
                                noNormals->draw(shadowProg);
                            }
                            Model->popMatrix();
                        }

                        // Thumb
                        Model->pushMatrix();
                        {
                            Model->translate(vec3(0.15, 0.0, 0.0));
                            Model->rotate(glm::radians(45.0f), vec3(0, 0, 1));
                            if (toggleAnimate)
                                Model->rotate(sTheta / 6, vec3(0, 0, 1));
                            Model->translate(vec3(0.1, 0.15, 0));
                            Model->scale(vec3(0.3, 0.06, 0.06));
                            setModel(shadowProg, Model);
                            noNormals->draw(shadowProg);
                        }
                        Model->popMatrix();
                    }
                    Model->popMatrix();

                    // Lower arm final scale
                    Model->scale(vec3(0.8, 0.3, 0.3));
                    setModel(shadowProg, Model);
                    noNormals->draw(shadowProg);
                }
                Model->popMatrix();

                // Upper arm final scale
                Model->scale(vec3(0.8, 0.25, 0.25));
                setModel(shadowProg, Model);
                noNormals->draw(shadowProg);
            }
            Model->popMatrix();

            // Left arm
            Model->pushMatrix();
            {
                Model->translate(vec3(-0.9, 0.8, 0));
                Model->rotate(glm::radians(50.0f), vec3(0, 0, 1));
                Model->rotate(glm::radians(0.0f) + gRot, vec3(0, 1, 0));
                Model->translate(vec3(-0.8, 0, 0));

                // Lower arm
                Model->pushMatrix();
                {
                    Model->translate(vec3(-0.6, 0, 0));
                    Model->rotate(glm::radians(25.0f), vec3(0, 0, 1));
                    Model->translate(vec3(-0.6, 0, 0));

                    // Hand
                    Model->pushMatrix();
                    {
                        Model->translate(vec3(-0.5, 0, 0));
                        Model->rotate(glm::radians(15.0f), vec3(0, 0, 1));
                        Model->translate(vec3(-0.5, 0, 0));

                        // Fingers
                        for (int i = 0; i < 4; i++)
                        {
                            Model->pushMatrix();
                            {
                                float angle = glm::radians(20.0f - i * 17.5f);
                                Model->translate(vec3(-0.05, 0, 0));
                                Model->rotate(angle, vec3(0, 0, 1));

                                Model->translate(vec3(-0.25, 0, 0));
                                Model->scale(vec3(0.3, 0.06, 0.06));
                                setModel(shadowProg, Model);
                                noNormals->draw(shadowProg);
                            }
                            Model->popMatrix();
                        }

                        // Thumb
                        Model->pushMatrix();
                        {
                            Model->translate(vec3(-0.15, 0.0, 0.0));
                            Model->rotate(glm::radians(45.0f), vec3(0, 0, 1));

                            Model->translate(vec3(-0.1, -0.15, 0));
                            Model->scale(vec3(0.3, 0.06, 0.06));
                            setModel(shadowProg, Model);
                            noNormals->draw(shadowProg);
                        }
                        Model->popMatrix();

                        Model->scale(vec3(-0.4, 0.25, 0.15));
                        setModel(shadowProg, Model);
                        noNormals->draw(shadowProg);
                    }
                    Model->popMatrix();

                    Model->scale(vec3(-0.8, 0.3, 0.3));
                    setModel(shadowProg, Model);
                    noNormals->draw(shadowProg);
                }
                Model->popMatrix();

                Model->scale(vec3(-0.8, 0.25, 0.25));
                setModel(shadowProg, Model);
                noNormals->draw(shadowProg);
            }
            Model->popMatrix();

            // Leg
            Model->pushMatrix();
            {
                Model->translate(vec3(0.5, -1.25, 0));
                if (toggleAnimate)
                    Model->rotate(sTheta, vec3(1, 0, 0));
                Model->rotate(glm::radians(2.0f), vec3(0, 0, 1));
                Model->translate(vec3(0.1, -0.75, 0));

                // Foot
                Model->pushMatrix();
                {
                    Model->translate(vec3(0, -1.35, 0.2));

                    Model->scale(vec3(0.3, 0.2, 0.6));
                    setModel(shadowProg, Model);
                    noNormals->draw(shadowProg);
                }
                Model->popMatrix();

                Model->scale(vec3(0.45, 1.5, 0.8));
                setModel(shadowProg, Model);
                noNormals->draw(shadowProg);
            }
            Model->popMatrix();

            // O
            Model->pushMatrix();
            {
                Model->translate(vec3(-0.5, -1.25, 0));
                if (toggleAnimate)
                    Model->rotate(-sTheta, vec3(1, 0, 0));
                Model->rotate(-glm::radians(2.0f), vec3(0, 0, 1));
                Model->translate(vec3(-0.1, -0.75, 0));

                Model->pushMatrix();
                {
                    Model->translate(vec3(0, -1.35, 0.2));

                    Model->scale(vec3(0.3, 0.2, 0.6));
                    setModel(shadowProg, Model);
                    noNormals->draw(shadowProg);
                }
                Model->popMatrix();

                Model->scale(vec3(0.45, 1.5, 0.8));
                setModel(shadowProg, Model);
                noNormals->draw(shadowProg);
            }
            Model->popMatrix();
        }
        Model->popMatrix();

        shadowProg->unbind();
        prog->bind();
    }

    void drawMultiMesh(shared_ptr<Program> prog, const vector<shared_ptr<Shape>> &meshes)
    {
        for (auto &shape : meshes)
        {
            if (shape)
                shape->draw(prog);
        }
    }

    void drawMultiMeshShadow(shared_ptr<MatrixStack> Model, shared_ptr<Program> shadowProg, const vector<shared_ptr<Shape>> &meshes, shared_ptr<MatrixStack> Projection, shared_ptr<MatrixStack> View)
    {
        shadowProg->bind();

        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        glUniformMatrix4fv(shadowProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));

        Model->pushMatrix();
        {
            const float epsilon = 0.001f;
            Model->translate(vec3(0.0f, g_groundY + epsilon, 0.0f));
            Model->scale(vec3(1.0f, 0.0f, 1.0f));
            setModel(shadowProg, Model);
            glDisable(GL_DEPTH_TEST);
            for (auto &shape : meshes)
            {
                if (shape)
                    shape->draw(shadowProg);
            }
            glEnable(GL_DEPTH_TEST);
        }
        Model->popMatrix();

        shadowProg->unbind();
        prog->bind();
    }

    void render()
    {
        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        glViewport(0, 0, width, height);

        // Clear framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the matrix stack for Lab 6
        float aspect = width / (float)height;

        // Create the matrix stacks - please leave these alone for now
        auto Projection = make_shared<MatrixStack>();
        auto View = make_shared<MatrixStack>();
        auto Model = make_shared<MatrixStack>();

        // Apply perspective projection.
        Projection->pushMatrix();
        Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

        // View is global translation along negative z for now
        View->pushMatrix();
        View->loadIdentity();
        // camera up and down
        View->translate(vec3(0, gCamH, 0));
        // global rotate (the whole scene )
        View->rotate(gRot, vec3(0, 1, 0));

        glm::vec3 dir;
        dir.x = radius * cos(phi) * cos(theta);
        dir.y = radius * sin(phi);
        dir.z = radius * cos(phi) * cos(glm::pi<float>() / 2.0f - theta);
        glm::vec3 target = eye - dir;
        glm::vec3 up = glm::vec3(0, 1, 0);
        View->lookAt(eye, target, up);

        // switch shaders to the texture mapping shader and draw the ground
        texProg->bind();
        glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
        glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));

        drawGround(texProg);

        texProg->unbind();

        // Draw the scene
        prog->bind();
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
        SetLight(prog);

        // draw the array of bunnies
        Model->pushMatrix();

        vector<vector<shared_ptr<Shape>>> pokemon = {eeveeMeshes, dittoMeshes, snorlaxMeshes, bulbasaurMeshes};
        vector<vector<shared_ptr<pair<vec3, vec3>>>> pokemonMinsMaxs = {eeveeMinsMaxs, dittoMinsMaxs, snorlaxMinsMaxs, bulbasaurMinsMaxs};

        float sp = 3.0f;
        float off = -3.5f;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                int idx = (i + j) % 4;
                auto &meshes = pokemon[idx];
                if (meshes.empty())
                    continue;

                Model->pushMatrix();
                {
                    Model->translate(vec3(off + sp * i, -1, off + sp * j));
                    Model->scale(vec3(0.85f));
                    SetMaterial(prog, idx);
                    Model->pushMatrix();
                    {
                        shadowProg->bind();
                        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
                        glUniformMatrix4fv(shadowProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
                        const float epsilon = 0.001f;
                        Model->translate(vec3(0.0f, g_groundY + epsilon, 0.0f));
                        Model->scale(vec3(1.0f, 0.0f, 1.0f));

                        for (auto &mesh : meshes)
                        {
                            if (!mesh)
                                continue;

                            Model->pushMatrix();
                            {
                                // SAME transform used for drawing
                                if (idx == 0 || idx == 3)
                                {
                                    Model->translate(vec3(0, 1, 0));
                                    Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                                }

                                Model->multMatrix(
                                    mesh->getNormalizationMatrix(
                                        pokemonMinsMaxs[idx][0]->first,
                                        pokemonMinsMaxs[idx][0]->second));

                                setModel(shadowProg, Model);
                                mesh->draw(shadowProg);
                            }
                            Model->popMatrix();
                        }

                        shadowProg->unbind();
                        prog->bind();
                    }
                    Model->popMatrix();
                    for (auto &mesh : meshes)
                    {
                        Model->pushMatrix();
                        {
                            if (idx == 0 || idx == 3)
                            {
                                Model->translate(vec3(0, 1, 0));
                                Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                            }
                            Model->multMatrix(mesh->getNormalizationMatrix(pokemonMinsMaxs[idx][0]->first, pokemonMinsMaxs[idx][0]->second));
                            glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
                            drawMultiMesh(prog, {mesh});
                        }
                        Model->popMatrix();
                    }
                }
                Model->popMatrix();
            }
        }
        Model->popMatrix();

        // draw the waving HM
        SetMaterial(prog, 4);
        drawHierModel(Model);
        drawHierModelShadow(Model);

        prog->unbind();

        // animation update example
        sTheta = sin(glfwGetTime());
        eTheta = std::max(0.0f, (float)sin(glfwGetTime()));
        hTheta = std::max(0.0f, (float)cos(glfwGetTime()));

        // Pop matrix stacks.
        Projection->popMatrix();
        View->popMatrix();
    }
};

int main(int argc, char *argv[])
{
    // Where the resources are loaded from
    std::string resourceDir = "../resources";

    if (argc >= 2)
    {
        resourceDir = argv[1];
    }

    Application *application = new Application();

    // Your main will always include a similar set up to establish your window
    // and GL context, etc.

    WindowManager *windowManager = new WindowManager();
    windowManager->init(640, 480);
    windowManager->setEventCallbacks(application);
    application->windowManager = windowManager;

    // This is the code that will likely change program to program as you
    // may need to initialize or set up different data and state

    application->init(resourceDir);
    application->initGeom(resourceDir);

    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(windowManager->getHandle()))
    {
        // Render scene.
        application->render();

        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }

    // Quit program.
    windowManager->shutdown();
    return 0;
}
