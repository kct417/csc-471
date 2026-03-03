/*
 * Program 4 example with diffuse and spline camera PRESS 'g'
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn (spline D. McGirr)
 */

#include <iostream>
#include <chrono>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "Bezier.h"
#include "Spline.h"
#include "ModelUtils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>
#define PI 3.1415927

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:
    WindowManager *windowManager = nullptr;

    // Our shader program - use this one for Blinn-Phong has diffuse
    std::shared_ptr<Program> prog;

    // shadow shader programs
    std::shared_ptr<Program> shadowProg;

    // Our shader program for textures
    std::shared_ptr<Program> texProg;

    // our geometry
    shared_ptr<Shape> sphere;

    shared_ptr<Shape> theDog;

    shared_ptr<Shape> cube;

    vector<shared_ptr<Shape>> eeveeModel;
    vector<shared_ptr<Shape>> dittoModel;
    vector<shared_ptr<Shape>> snorlaxModel;
    vector<shared_ptr<Shape>> bulbasaurModel;

    // global data for ground plane - direct load constant defined CPU data to GPU (not obj)
    GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
    int g_GiboLen;
    // ground VAO
    GLuint GroundVertexArrayID;

    // the image to use as a texture (ground)
    shared_ptr<Texture> texture0;
    shared_ptr<Texture> texture1;
    shared_ptr<Texture> texture2;

    // animation data
    float lightTrans = 0;
    float animationTheta = 0;
    float g_groundSize = 20;
    float g_groundY = -0.25;
    bool toggleMaterial = false;
    bool toggleAnimation = false;

    // camera
    double g_phi, g_theta;
    vec3 view = vec3(0, 0, 1);
    vec3 strafe = vec3(1, 0, 0);
    vec3 g_eye = vec3(0, 1, 0);
    vec3 g_lookAt = vec3(0, 1, -4);
    vec3 g_up = vec3(0, 1, 0);
    vec3 movementInput = vec3(0);
    float g_sensitivty = 0.05f;
    float cameraRadius = 0.4f;

    Spline splinepath[8];
    bool goCamera = false;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            lightTrans += 0.5;
        }
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            lightTrans -= 0.5;
        }
        if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        if (key == GLFW_KEY_G && action == GLFW_RELEASE)
        {
            goCamera = !goCamera;
        }
        if (key == GLFW_KEY_M && action == GLFW_PRESS)
        {
            toggleMaterial = !toggleMaterial;
        }
        if (key == GLFW_KEY_X && action == GLFW_PRESS)
        {
            toggleAnimation = !toggleAnimation;
        }

        view = glm::normalize(g_eye - g_lookAt);
        strafe = glm::cross(view, g_up);

        if (key == GLFW_KEY_A && action == GLFW_PRESS)
            movementInput.x = 1;
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
            movementInput.x = -1;
        if (key == GLFW_KEY_W && action == GLFW_PRESS)
            movementInput.z = -1;
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
            movementInput.z = 1;
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
        cout << "xDel + yDel " << deltaX << " " << deltaY << endl;
        // fill in for game camera
        g_theta -= deltaX * g_sensitivty;
        g_phi -= deltaY * g_sensitivty;

        float limit = glm::radians(80.0f);
        g_phi = glm::clamp(static_cast<float>(g_phi), -limit, limit);
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

        g_theta = -PI / 2.0;

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
        prog->addAttribute("vertTex"); // silence error

        shadowProg = make_shared<Program>();
        shadowProg->setVerbose(true);
        shadowProg->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/shadow_frag.glsl");
        shadowProg->init();
        shadowProg->addUniform("P");
        shadowProg->addUniform("V");
        shadowProg->addUniform("M");
        shadowProg->addUniform("lightPos");
        shadowProg->addUniform("lightColor");
        shadowProg->addAttribute("vertPos");
        shadowProg->addAttribute("vertNor");
        shadowProg->addAttribute("vertTex");

        // Initialize the GLSL program that we will use for texture mapping
        texProg = make_shared<Program>();
        texProg->setVerbose(true);
        texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
        texProg->init();
        texProg->addUniform("P");
        texProg->addUniform("V");
        texProg->addUniform("M");
        texProg->addUniform("flip");
        texProg->addUniform("Texture0");
        texProg->addUniform("MatShine");
        texProg->addUniform("lightPos");
        texProg->addAttribute("vertPos");
        texProg->addAttribute("vertNor");
        texProg->addAttribute("vertTex");

        // read in a load the texture
        texture0 = make_shared<Texture>();
        texture0->setFilename(resourceDirectory + "/animeGrass.jpg");
        texture0->init();
        texture0->setUnit(0);
        texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        texture1 = make_shared<Texture>();
        texture1->setFilename(resourceDirectory + "/skybox/sky.png");
        texture1->init();
        texture1->setUnit(1);
        texture1->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        texture2 = make_shared<Texture>();
        texture2->setFilename(resourceDirectory + "/cartoonWood.jpg");
        texture2->init();
        texture2->setUnit(2);
        texture2->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

        // init splines up and down
        float orbitRadius = 13.0f;
        float orbitHeight = 4.0f;
        float heightAmplitude = 2.0f;
        int numSplines = 8;

        std::vector<glm::vec3> p0(numSplines), p1(numSplines), p2(numSplines), p3(numSplines);

        float tangentScale = orbitRadius * 0.25f;
        for (int i = 0; i < numSplines; i++)
        {
            float angle0 = glm::two_pi<float>() * i / numSplines;
            float angle1 = glm::two_pi<float>() * ((i + 1) % numSplines) / numSplines;

            float y0 = orbitHeight + sin(angle0 * 2.0f) * heightAmplitude;
            float y3 = orbitHeight + sin(angle1 * 2.0f) * heightAmplitude;

            glm::vec3 p0 = glm::vec3(orbitRadius * cos(angle0), y0, orbitRadius * sin(angle0));
            glm::vec3 p3 = glm::vec3(orbitRadius * cos(angle1), y3, orbitRadius * sin(angle1));

            glm::vec3 tangent0 = glm::normalize(glm::vec3(-sin(angle0), 0, cos(angle0))) * tangentScale;
            glm::vec3 tangent1 = glm::normalize(glm::vec3(-sin(angle1), 0, cos(angle1))) * tangentScale;

            float yOffset1 = (y0 + y3) / 2.0f + sin(angle0 * 2.0f) * heightAmplitude;
            float yOffset2 = (y0 + y3) / 2.0f + sin(angle1 * 2.0f) * heightAmplitude;

            glm::vec3 p1 = p0 + tangent0 + glm::vec3(0, yOffset1 - y0, 0);
            glm::vec3 p2 = p3 - tangent1 + glm::vec3(0, yOffset2 - y3, 0);

            splinepath[i] = Spline(p0, p1, p2, p3, 5.0f);
        }
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
        bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
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

        // Initialize bunny mesh.
        vector<tinyobj::shape_t> TOshapesB;
        vector<tinyobj::material_t> objMaterialsB;
        // load in the mesh and make the shape(s)
        rc = tinyobj::LoadObj(TOshapesB, objMaterialsB, errStr, (resourceDirectory + "/dog.obj").c_str());
        if (!rc)
        {
            cerr << errStr << endl;
        }
        else
        {
            theDog = make_shared<Shape>();
            theDog->createShape(TOshapesB[0]);
            theDog->measure();
            theDog->init();
        }

        eeveeModel = loadMultModel("/meshes/eevee.obj", resourceDirectory);
        dittoModel = loadMultModel("/meshes/ditto.obj", resourceDirectory);
        snorlaxModel = loadMultModel("/meshes/snorlax.obj", resourceDirectory);
        bulbasaurModel = loadMultModel("/meshes/bulbasaur.obj", resourceDirectory);

        // code to load in the ground plane (CPU defined data passed to GPU)
        initGround();
    }

    vector<shared_ptr<Shape>> loadMultModel(const std::string &filename, const std::string &resourceDirectory)
    {
        vector<shared_ptr<Shape>> model;
        vector<tinyobj::shape_t> TOshapes;
        vector<tinyobj::material_t> objMaterials;
        string errStr;

        bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + filename).c_str());
        if (!rc)
        {
            cerr << "Failed to load " << filename << ": " << errStr << endl;
            return model;
        }

        for (auto &TOshape : TOshapes)
        {
            auto shape = make_shared<Shape>();
            shape->createShape(TOshape);
            shape->measure();
            shape->init();
            model.push_back(shape);
        }

        return model;
    };

    // directly pass quad for the ground to the GPU
    void initGround()
    {

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
        SetModelTransforms(vec3(0, -1, 0), 0, 0, 1, curS);
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
        }
    }

    void SetLight(shared_ptr<Program> curS)
    {
        glUniform3f(curS->getUniform("lightPos"), -2.0f + lightTrans, 2.0f, 2.0f);
        glUniform3f(curS->getUniform("lightColor"), 1.0f, 1.0f, 1.0f);
    }

    /* helper function to set model trasnforms */
    void SetModelTransforms(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS)
    {
        mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
        mat4 RotX = glm::rotate(glm::mat4(1.0f), rotX, vec3(1, 0, 0));
        mat4 RotY = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
        mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
        mat4 ctm = Trans * RotX * RotY * ScaleS;
        glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
    }

    void SetModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack> M)
    {
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    }

    /* camera controls - do not change */
    void SetView(shared_ptr<Program> shader)
    {
        glm::mat4 Cam = glm::lookAt(g_eye, g_lookAt, vec3(0, 1, 0));
        glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
    }

    /* code to draw waving hierarchical model */
    void drawHierModel(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog)
    {
        Model->pushMatrix();
        {
            Model->translate(vec3(0, 2.5, -12));

            // head
            Model->pushMatrix();
            {
                Model->translate(vec3(0, 1.4, 0));
                Model->scale(vec3(0.5, 0.5, 0.5));
                SetModel(prog, Model);
                sphere->draw(prog);
            }
            Model->popMatrix();

            // torso
            Model->pushMatrix();
            {
                Model->scale(vec3(1.15, 1.35, 1.25));
                SetModel(prog, Model);
                sphere->draw(prog);
            }
            Model->popMatrix();

            // right arm
            Model->pushMatrix();
            {
                Model->translate(vec3(0.8, 0.8, 0));
                if (toggleAnimation)
                    Model->rotate(animationTheta / 4, vec3(0, 0, 1));
                Model->rotate(glm::radians(30.0f), vec3(0, 0, 1));
                Model->translate(vec3(0.8, 0, 0));

                // lower arm
                Model->pushMatrix();
                {
                    Model->translate(vec3(0.6, 0, 0));
                    if (toggleAnimation)
                        Model->rotate(animationTheta / 4, vec3(0, 0, 1));
                    Model->rotate(glm::radians(30.0f), vec3(0, 0, 1));
                    Model->translate(vec3(0.6, 0, 0));

                    // hand
                    Model->pushMatrix();
                    {
                        Model->translate(vec3(0.5, 0, 0));
                        if (toggleAnimation)
                            Model->rotate(animationTheta / 8, vec3(0, 0, 1));
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
                                if (toggleAnimation)
                                    Model->rotate(animationTheta / 6, vec3(0, 0, 1));

                                Model->translate(vec3(0.25, 0, 0));
                                Model->scale(vec3(0.3, 0.06, 0.06));
                                SetModel(prog, Model);
                                sphere->draw(prog);
                            }
                            Model->popMatrix();
                        }

                        // thumb
                        Model->pushMatrix();
                        {
                            Model->translate(vec3(0.15, 0.0, 0.0));
                            Model->rotate(glm::radians(45.0f), vec3(0, 0, 1));
                            if (toggleAnimation)
                                Model->rotate(animationTheta / 6, vec3(0, 0, 1));

                            Model->translate(vec3(0.1, 0.15, 0));
                            Model->scale(vec3(0.3, 0.06, 0.06));
                            SetModel(prog, Model);
                            sphere->draw(prog);
                        }
                        Model->popMatrix();

                        Model->scale(vec3(0.4, 0.25, 0.15));
                        SetModel(prog, Model);
                        sphere->draw(prog);
                    }
                    Model->popMatrix();

                    Model->scale(vec3(0.8, 0.3, 0.3));
                    SetModel(prog, Model);
                    sphere->draw(prog);
                }
                Model->popMatrix();

                Model->scale(vec3(0.8, 0.25, 0.25));
                SetModel(prog, Model);
                sphere->draw(prog);
            }
            Model->popMatrix();

            // left arm
            Model->pushMatrix();
            {
                Model->translate(vec3(-0.9, 0.8, 0));
                Model->rotate(glm::radians(50.0f), vec3(0, 0, 1));
                Model->translate(vec3(-0.8, 0, 0));

                // lower arm
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
                                SetModel(prog, Model);
                                sphere->draw(prog);
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
                            SetModel(prog, Model);
                            sphere->draw(prog);
                        }
                        Model->popMatrix();

                        Model->scale(vec3(-0.4, 0.25, 0.15));
                        SetModel(prog, Model);
                        sphere->draw(prog);
                    }
                    Model->popMatrix();

                    Model->scale(vec3(-0.8, 0.3, 0.3));
                    SetModel(prog, Model);
                    sphere->draw(prog);
                }
                Model->popMatrix();

                Model->scale(vec3(-0.8, 0.25, 0.25));
                SetModel(prog, Model);
                sphere->draw(prog);
            }
            Model->popMatrix();

            // right leg
            Model->pushMatrix();
            {
                Model->translate(vec3(0.5, -1.25, 0));
                if (toggleAnimation)
                    Model->rotate(animationTheta, vec3(1, 0, 0));
                Model->rotate(glm::radians(2.0f), vec3(0, 0, 1));
                Model->translate(vec3(0.1, -0.75, 0));

                // foot
                Model->pushMatrix();
                {
                    Model->translate(vec3(0, -1.35, 0.2));

                    Model->scale(vec3(0.3, 0.2, 0.6));
                    SetModel(prog, Model);
                    sphere->draw(prog);
                }
                Model->popMatrix();

                Model->scale(vec3(0.45, 1.5, 0.8));
                SetModel(prog, Model);
                sphere->draw(prog);
            }
            Model->popMatrix();

            // left leg
            Model->pushMatrix();
            {
                Model->translate(vec3(-0.5, -1.25, 0));
                if (toggleAnimation)
                    Model->rotate(-animationTheta, vec3(1, 0, 0));
                Model->rotate(-glm::radians(2.0f), vec3(0, 0, 1));
                Model->translate(vec3(-0.1, -0.75, 0));

                // foot
                Model->pushMatrix();
                {
                    Model->translate(vec3(0, -1.35, 0.2));

                    Model->scale(vec3(0.3, 0.2, 0.6));
                    SetModel(prog, Model);
                    sphere->draw(prog);
                }
                Model->popMatrix();

                Model->scale(vec3(0.45, 1.5, 0.8));
                SetModel(prog, Model);
                sphere->draw(prog);
            }
            Model->popMatrix();
        }
        Model->popMatrix();
    }

    void drawMultiMesh(shared_ptr<Program> prog, const vector<shared_ptr<Shape>> &meshes)
    {
        for (auto &shape : meshes)
        {
            if (shape)
                shape->draw(prog);
        }
    }

    void updateUsingCameraPath(float frametime)
    {
        if (goCamera)
        {
            g_lookAt = glm::vec3(0, 0, 0);
            for (auto &spline : splinepath)
            {
                if (!spline.isDone())
                {
                    spline.update(frametime);
                    g_eye = spline.getPosition();
                    break;
                }
            }
        }
    }

    void render(float frametime)
    {
        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        glViewport(0, 0, width, height);

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the matrix stack for Lab 6
        float aspect = width / (float)height;

        // Create the matrix stacks - please leave these alone for now
        auto Projection = make_shared<MatrixStack>();
        auto View = make_shared<MatrixStack>();
        auto Model = make_shared<MatrixStack>();

        view = glm::normalize(g_eye - g_lookAt);
        strafe = glm::cross(view, g_up);

        glm::vec3 eye = g_eye;

        float speed = g_sensitivty * 10.0f;

        eye += strafe * movementInput.x * speed;
        eye += view * movementInput.z * speed;

        glm::vec3 view;
        view.x = cos(g_phi) * cos(g_theta);
        view.y = sin(g_phi);
        view.z = cos(g_phi) * cos(glm::pi<float>() / 2.0f - g_theta);

        g_eye = eye;
        g_lookAt = g_eye - view;

        // update the camera position
        updateUsingCameraPath(frametime);

        // Apply perspective projection.
        Projection->pushMatrix();
        Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

        // Draw the doggos
        texProg->bind();
        glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        SetView(texProg);
        glUniform3f(texProg->getUniform("lightPos"), 2.0 + lightTrans, 2.0, 2.9);
        glUniform1f(texProg->getUniform("MatShine"), 27.9);
        Model->pushMatrix();

        // draw big background sphere
        glUniform1i(texProg->getUniform("flip"), 0);
        texture1->bind(texProg->getUniform("Texture0"));
        Model->pushMatrix();
        Model->loadIdentity();
        Model->scale(vec3(15.0));
        SetModel(texProg, Model);
        sphere->draw(texProg);
        Model->popMatrix();

        // draw the waving HM
        glUniform1i(texProg->getUniform("flip"), 1);
        texture2->bind(texProg->getUniform("Texture0"));
        Model->pushMatrix();
        Model->loadIdentity();
        drawHierModel(Model, texProg);
        Model->popMatrix();

        glUniform1i(texProg->getUniform("flip"), 1);
        drawGround(texProg);

        texProg->unbind();

        // use the material shader
        // set up all the matrices
        prog->bind();
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        SetView(prog);
        SetLight(prog);
        glUniform3f(prog->getUniform("lightPos"), 2.0 + lightTrans, 2.0, 2.9);

        float sp = 5.0;
        float off = -1.5;
        std::vector<std::vector<std::shared_ptr<Shape>>> models = {eeveeModel, dittoModel, snorlaxModel, bulbasaurModel};
        std::vector<std::pair<glm::vec3, glm::vec3>> modelMinMaxes(models.size());
        modelMinMaxes[0] = ModelUtils::measureModel(eeveeModel);
        modelMinMaxes[1] = ModelUtils::measureModel(dittoModel);
        modelMinMaxes[2] = ModelUtils::measureModel(snorlaxModel);
        modelMinMaxes[3] = ModelUtils::measureModel(bulbasaurModel);
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                int idx = (i + j) % 4;
                Model->pushMatrix();
                {
                    Model->translate(vec3(off + sp * i, -1, off + sp * j));
                    Model->scale(vec3(0.85f));
                    Model->rotate(glm::radians(-30.0f * i), vec3(0, 1, 0));
                    SetMaterial(prog, idx);
                    auto model = models[idx];
                    Model->pushMatrix();
                    {
                        if (idx == 0 || idx == 3)
                        {
                            Model->translate(vec3(0, 1, 0));
                            Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                        }
                        if (idx == 2)
                        {
                            Model->scale(vec3(2.0f, 2.0f, 2.0f));
                        }
                        Model->multMatrix(ModelUtils::getNormalizationMatrix(modelMinMaxes[idx].first, modelMinMaxes[idx].second));
                        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
                        drawMultiMesh(prog, model);
                    }
                    Model->popMatrix();
                }
                Model->popMatrix();
            }
        }

        prog->unbind();

        shadowProg->bind();
        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        SetView(shadowProg);
        SetLight(shadowProg);
        glUniform3f(shadowProg->getUniform("lightPos"), 2.0 + lightTrans, 2.0, 2.9);

        const float epsilon = 0.001f;

        Model->pushMatrix();
        {
            Model->translate(vec3(0.0f, g_groundY + epsilon - 1.0f, 0.0f));
            Model->scale(vec3(1.0f, 0.0f, 1.0f));
            drawHierModel(Model, shadowProg);
        }
        Model->popMatrix();

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                int idx = (i + j) % 4;
                Model->pushMatrix();
                {
                    Model->translate(vec3(0.0f, g_groundY + epsilon - 1.0f, 0.0f));
                    Model->scale(vec3(1.0f, 0.0f, 1.0f));
                    Model->translate(vec3(off + sp * i, -1, off + sp * j));
                    Model->scale(vec3(0.85f));
                    Model->rotate(glm::radians(-30.0f * i), vec3(0, 1, 0));
                    auto model = models[idx];
                    Model->pushMatrix();
                    {
                        if (idx == 0 || idx == 3)
                        {
                            Model->translate(vec3(0, 1, 0));
                            Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                        }
                        if (idx == 2)
                        {
                            Model->scale(vec3(2.0f, 2.0f, 2.0f));
                        }
                        Model->multMatrix(ModelUtils::getNormalizationMatrix(modelMinMaxes[idx].first, modelMinMaxes[idx].second));
                        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
                        drawMultiMesh(shadowProg, model);
                    }
                    Model->popMatrix();
                }
                Model->popMatrix();
            }
        }

        shadowProg->unbind();

        // update animation data
        animationTheta = sin(glfwGetTime());
        movementInput = glm::vec3(0.0f);

        // Pop matrix stacks.
        Projection->popMatrix();
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

    auto lastTime = chrono::high_resolution_clock::now();
    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(windowManager->getHandle()))
    {
        // save current time for next frame
        auto nextLastTime = chrono::high_resolution_clock::now();

        // get time since last frame
        float deltaTime =
            chrono::duration_cast<std::chrono::microseconds>(
                chrono::high_resolution_clock::now() - lastTime)
                .count();
        // convert microseconds (weird) to seconds (less weird)
        deltaTime *= 0.000001;

        // reset lastTime so that we can calculate the deltaTime
        // on the next frame
        lastTime = nextLastTime;

        // Render scene.
        application->render(deltaTime);
        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }

    // Quit program.
    windowManager->shutdown();
    return 0;
}
