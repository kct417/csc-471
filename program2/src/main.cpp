/*
 * Example two meshes and two shaders (could also be used for Program 2)
 * includes modifications to shape and initGeom in preparation to load
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

    // Our shader program
    std::shared_ptr<Program> prog;

    // Our shader program
    std::shared_ptr<Program> solidColorProg;

    // Shape to be used (from  file) - modify to support multiple
    vector<shared_ptr<Shape>> eevee;
    vector<shared_ptr<Shape>> umbreon;

    // example data that might be useful when trying to compute bounds on multi-shape
    vec3 gMin;

    // animation data
    float sTheta = 0;
    float gTrans = 0;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            gTrans -= 0.2;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            gTrans += 0.2;
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

    void resizeCallback(GLFWwindow *window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void init(const std::string &resourceDirectory)
    {
        GLSL::checkVersion();

        // Set background color.
        glClearColor(.12f, .34f, .56f, 1.0f);
        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);

        // Initialize the GLSL program.
        prog = make_shared<Program>();
        prog->setVerbose(true);
        prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
        prog->init();
        prog->addUniform("P");
        prog->addUniform("V");
        prog->addUniform("M");
        prog->addAttribute("vertPos");
        prog->addAttribute("vertNor");

        // Initialize the GLSL program.
        solidColorProg = make_shared<Program>();
        solidColorProg->setVerbose(true);
        solidColorProg->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/solid_frag.glsl");
        solidColorProg->init();
        solidColorProg->addUniform("P");
        solidColorProg->addUniform("V");
        solidColorProg->addUniform("M");
        solidColorProg->addUniform("solidColor");
        solidColorProg->addAttribute("vertPos");
        solidColorProg->addAttribute("vertNor");
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
        bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/eevee.obj").c_str());

        if (!rc)
        {
            cerr << errStr << endl;
        }
        else
        {
            cout << "Warning there are this many shapes: " << TOshapes.size() << endl;
            for (int i = 0; i < TOshapes.size(); i++)
            {
                shared_ptr<Shape> mesh = make_shared<Shape>(false);
                mesh->createShape(TOshapes[i]);
                mesh->measure();
                mesh->init();
                eevee.push_back(mesh);
            }
        }

        // load in another mesh and make the shape(s)
        vector<tinyobj::shape_t> TOshapes2;
        rc = tinyobj::LoadObj(TOshapes2, objMaterials, errStr, (resourceDirectory + "/umbreon.obj").c_str());

        if (!rc)
        {
            cerr << errStr << endl;
        }
        else
        {
            // for now all our shapes will not have textures - change in later labs
            cout << "Warning there are this many shapes: " << TOshapes2.size() << endl;
            for (int i = 0; i < TOshapes2.size(); i++)
            {
                shared_ptr<Shape> mesh = make_shared<Shape>(false);
                mesh->createShape(TOshapes2[i]);
                mesh->measure();
                mesh->init();
                umbreon.push_back(mesh);
            }
        }

        // read out information stored in the shape about its size - something like this...
        // then do something with that information.....
        for (int i = 0; i < eevee.size(); i++)
        {
            shared_ptr<Shape> mesh = eevee[i];
            if (gMin.x > mesh->min.x)
                gMin.x = mesh->min.x;
            if (gMin.y > mesh->min.y)
                gMin.y = mesh->min.y;
        }
        for (int i = 0; i < umbreon.size(); i++)
        {
            shared_ptr<Shape> mesh = umbreon[i];
            if (gMin.x > mesh->min.x)
                gMin.x = mesh->min.x;
            if (gMin.y > mesh->min.y)
                gMin.y = mesh->min.y;
        }
    }

    /* helper for sending top of the matrix strack to GPU */
    void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack> M)
    {
        glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    }

    /* helper function to set model trasnforms */
    void setModel(shared_ptr<Program> curS, vec3 trans, vec3 sc, float rotX, float rotY, float rotZ)
    {
        mat4 Trans = glm::translate(glm::mat4(1.0f), trans);
        mat4 RotX = glm::rotate(glm::mat4(1.0f), rotX, vec3(1, 0, 0));
        mat4 RotY = glm::rotate(glm::mat4(1.0f), rotY, vec3(0, 1, 0));
        mat4 RotZ = glm::rotate(glm::mat4(1.0f), rotZ, vec3(0, 0, 1));
        mat4 ScaleS = glm::scale(glm::mat4(1.0f), sc);
        mat4 ctm = Trans * RotX * RotY * RotZ * ScaleS;
        glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
    }

    void render()
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

        // Apply perspective projection.
        Projection->pushMatrix();
        Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

        // View is global translation along negative z for now
        View->pushMatrix();
        View->loadIdentity();
        View->translate(vec3(0, 0, -30));
        View->rotate(gTrans, vec3(0, 1, 0));
        View->translate(vec3(0, 0, 15));

        vec3 translateEevee = -eevee[0]->center;
        vec3 translateUmbreon = -umbreon[0]->center;
        vec3 min, max;
        min = vec3(1.1754E+38F);
        max = vec3(-1.1754E+38F);
        for (auto &mesh : eevee)
        {
            min = glm::min(min, mesh->min);
            max = glm::max(max, mesh->max);
        }
        translateEevee = -vec3(min.x + (max.x - min.x) / 2.0 + 10, min.y + (max.y - min.y) / 2.0, min.z + (max.z - min.z) / 2.0 + 10);
        min = vec3(1.1754E+38F);
        max = vec3(-1.1754E+38F);
        for (auto &mesh : umbreon)
        {
            min = glm::min(min, mesh->min);
            max = glm::max(max, mesh->max);
        }
        translateUmbreon = -vec3(min.x + (max.x - min.x) / 2.0 + 10, min.y + (max.y - min.y) / 2.0 - 5, min.z + (max.z - min.z) / 2.0 - 16);

        vec3 scaleEevee = vec3(5.0f / eevee[0]->largeExtent());
        vec3 scaleUmbreon = vec3(10.0f / umbreon[0]->largeExtent());
        vec3 scaleBaseEevee = scaleEevee;
        vec3 scaleBaseUmbreon = scaleUmbreon;
        for (auto &mesh : eevee)
        {
            if (mesh->largeExtent() > eevee[0]->largeExtent())
            {
                scaleEevee = vec3(5.0f / mesh->largeExtent());
                scaleBaseEevee = scaleEevee;
            }
        }
        for (auto &mesh : umbreon)
        {
            if (mesh->largeExtent() > umbreon[0]->largeExtent())
            {
                scaleUmbreon = vec3(10.0f / mesh->largeExtent());
                scaleBaseUmbreon = scaleUmbreon;
            }
        }

        // Eevee transform into Umbreon in 3 seconds
        float t = glfwGetTime();
        float durationEevee = 5.0f;
        float durationUmbreon = 5.0f;
        float totalDuration = durationEevee + durationUmbreon;
        float tClamped = glm::min(t, totalDuration);

        if (t < durationEevee)
        {
            scaleEevee = scaleEevee * (1.0f - t / durationEevee);
            translateEevee += vec3(tClamped, 0, tClamped);
        }
        else
            scaleEevee = vec3(0.0f);

        if (t >= durationEevee)
        {
            float tUmbreon = t - durationEevee;
            scaleUmbreon = scaleUmbreon * glm::clamp(tUmbreon / durationUmbreon, 0.0f, 1.0f);
            translateUmbreon += vec3(tClamped, 0, tClamped);
        }
        else
            scaleUmbreon = vec3(0.0f);

        // Draw a solid colored sphere
        solidColorProg->bind();
        // send the projetion and view for solid shader
        glUniformMatrix4fv(solidColorProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        glUniformMatrix4fv(solidColorProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));
        // send in the color to use
        glUniform3f(solidColorProg->getUniform("solidColor"), 0.1, 0.2, 0.5);

        // Eevee shadow
        vec3 shadowEeveeScale = scaleEevee;
        shadowEeveeScale.z = 0.01f;
        if (scaleEevee.x > 0.0f)
        {
            for (auto &mesh : eevee)
            {
                setModel(solidColorProg, translateEevee - vec3(0, 0.5, 0), shadowEeveeScale, glm::radians(-90.0f), glm::radians(0.0f), glm::radians(45.0f));
                mesh->draw(solidColorProg);
            }
        }

        // Umbreon shadow
        vec3 shadowUmbreonScale = scaleUmbreon;

        shadowUmbreonScale.z = 0.01f;
        if (scaleUmbreon.x > 0.0f)
        {
            for (auto &mesh : umbreon)
            {
                setModel(solidColorProg, translateUmbreon - vec3(0, 0.5, 0), shadowUmbreonScale, glm::radians(-90.0f), glm::radians(0.0f), glm::radians(45.0f));
                mesh->draw(solidColorProg);
            }
        }

        solidColorProg->unbind();

        // Draw base Hierarchical person
        prog->bind();
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix()));

        // use helper function that uses glm to create some transform matrices
        // Draw Eevee only if still shrinking
        if (scaleEevee.x > 0.0f)
        {
            for (auto &mesh : eevee)
            {
                setModel(prog, translateEevee, scaleEevee,
                         glm::radians(-90.0f), 0.0f, glm::radians(45.0f));
                mesh->draw(prog);
            }
        }

        for (auto &mesh : eevee)
        {
            setModel(prog, vec3(4, -4, 0), scaleBaseEevee * 0.5f,
                     glm::radians(-90.0f), 0.0f, glm::radians(-45.0f));
            mesh->draw(prog);
        }

        for (auto &mesh : eevee)
        {
            setModel(prog, vec3(-2, 3, -20), scaleBaseEevee * 0.5f,
                     glm::radians(-90.0f), 0.0f, glm::radians(45.0f));
            mesh->draw(prog);
        }

        // Draw Umbreon only after Eevee disappears
        if (scaleUmbreon.x > 0.0f)
        {
            for (auto &mesh : umbreon)
            {
                setModel(prog, translateUmbreon, scaleUmbreon,
                         glm::radians(-90.0f), 0.0f, glm::radians(45.0f));
                mesh->draw(prog);
            }
        }

        for (auto &mesh : umbreon)
        {
            setModel(prog, vec3(-12, -4, -18), scaleBaseUmbreon * 0.5f,
                     glm::radians(-90.0f), 0.0f, glm::radians(-45.0f));
            mesh->draw(prog);
        }

        for (auto &mesh : umbreon)
        {
            setModel(prog, vec3(8, -3, -10), scaleBaseUmbreon * 0.5f,
                     glm::radians(-90.0f), 0.0f, glm::radians(45.0f));
            mesh->draw(prog);
        }

        prog->unbind();

        // animation update example
        sTheta = sin(glfwGetTime());

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
