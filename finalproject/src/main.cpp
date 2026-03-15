/*
 * Program 4 example with diffuse and spline camera PRESS 'g'
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn (spline D. McGirr)
 */

#include <iostream>
#include <fstream>
#include <sstream>
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
#include "particleSys.h"

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

    std::shared_ptr<Program> prog;
    std::shared_ptr<Program> shadowProg;
    std::shared_ptr<Program> texProg;
    std::shared_ptr<Program> partProg;

    // our geometry
    shared_ptr<Shape> sphere;
    vector<shared_ptr<Shape>> treeModel;
    vector<shared_ptr<Shape>> bulbasaurModel;
    vector<shared_ptr<Shape>> dittoModel;
    vector<shared_ptr<Shape>> eeveeModel;
    vector<shared_ptr<Shape>> lucarioModel;
    vector<shared_ptr<Shape>> mewModel;
    vector<shared_ptr<Shape>> snorlaxModel;
    vector<shared_ptr<Shape>> umbreonModel;

    vector<tinyobj::material_t> sphereMaterials;
    vector<tinyobj::material_t> treeMaterials;
    vector<tinyobj::material_t> bulbasaurMaterials;
    vector<tinyobj::material_t> dittoMaterials;
    vector<tinyobj::material_t> eeveeMaterials;
    vector<tinyobj::material_t> lucarioMaterials;
    vector<tinyobj::material_t> mewMaterials;
    vector<tinyobj::material_t> snorlaxMaterials;
    vector<tinyobj::material_t> umbreonMaterials;

    std::vector<std::vector<std::shared_ptr<Shape>>> models;
    std::vector<std::pair<glm::vec3, glm::vec3>> modelMinMaxes;
    std::vector<glm::mat4> modelNormMats;

    std::shared_ptr<particleSys> particleSystem;

    enum TileType
    {
        TREE,
        BULBASAUR,
        DITTO,
        EEVEE,
        LUCARIO,
        MEW,
        SNORLAX,
        UMBREON,
        GRASS,
    };

    vector<vector<TileType>> worldGrid;
    float tileSize = 5.0f;

    // global data for ground plane - direct load constant defined CPU data to GPU (not obj)
    GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
    int g_GiboLen;
    // ground VAO
    GLuint GroundVertexArrayID;

    // the image to use as a texture (ground)
    shared_ptr<Texture> texture0;
    shared_ptr<Texture> texture1;
    shared_ptr<Texture> texture2;
    shared_ptr<Texture> texture3;

    // animation data
    float lightTransX = 0;
    float lightTransY = 0;
    float lightTransZ = 0;
    float animationTheta = 0;
    float g_groundSize = 20;
    float g_groundY = -0.25;
    bool toggleMaterial = false;
    bool toggleAnimation = false;

    // camera
    double g_phi, g_theta;
    vec3 view = vec3(0, 0, 1);
    vec3 vertical = vec3(0, 1, 0);
    vec3 strafe = vec3(1, 0, 0);
    vec3 g_eye = vec3(0, 1, 0);
    vec3 g_lookAt = vec3(0, 1, -4);
    vec3 g_up = vec3(0, 1, 0);
    vec3 movementInput = vec3(0);
    float g_sensitivty = 0.1f;
    float cameraRadius = 0.4f;

    Spline splinepath[8];
    bool goCamera = false;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        {
            lightTransX += 0.5;
        }
        if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        {
            lightTransX -= 0.5;
        }
        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            lightTransY += 0.5;
        }
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            lightTransY -= 0.5;
        }
        if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        {
            lightTransZ += 0.5;
        }
        if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        {
            lightTransZ -= 0.5;
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
        if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            particleSystem->reSet();
        }

        view = glm::normalize(g_eye - g_lookAt);
        strafe = glm::cross(view, g_up);

        if (key == GLFW_KEY_A)
        {
            if (action == GLFW_PRESS)
                movementInput.x = 1;
            else if (action == GLFW_RELEASE)
                movementInput.x = 0;
        }
        if (key == GLFW_KEY_D)
        {
            if (action == GLFW_PRESS)
                movementInput.x = -1;
            else if (action == GLFW_RELEASE)
                movementInput.x = 0;
        }
        if (key == GLFW_KEY_W)
        {
            if (action == GLFW_PRESS)
                movementInput.z = -1;
            else if (action == GLFW_RELEASE)
                movementInput.z = 0;
        }
        if (key == GLFW_KEY_S)
        {
            if (action == GLFW_PRESS)
                movementInput.z = 1;
            else if (action == GLFW_RELEASE)
                movementInput.z = 0;
        }
        if (key == GLFW_KEY_SPACE)
        {
            if (action == GLFW_PRESS)
                movementInput.y = 1;
            else if (action == GLFW_RELEASE)
                movementInput.y = 0;
        }
        if (key == GLFW_KEY_LEFT_SHIFT)
        {
            if (action == GLFW_PRESS)
                movementInput.y = -1;
            else if (action == GLFW_RELEASE)
                movementInput.y = 0;
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
        cout << "xDel + yDel " << deltaX << " " << deltaY << endl;
        // fill in for game camera
        g_theta += deltaX * g_sensitivty;
        g_phi += deltaY * g_sensitivty;

        float limit = glm::radians(80.0f);
        g_phi = glm::clamp(static_cast<float>(g_phi), -limit, limit);
    }

    void resizeCallback(GLFWwindow *window, int width, int height)
    {
        CHECKED_GL_CALL(glViewport(0, 0, width, height));
    }

    void init(const std::string &resourceDirectory)
    {
        GLSL::checkVersion();

        // Set background color.
        glClearColor(.72f, .84f, 1.06f, 1.0f);
        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);
        CHECKED_GL_CALL(glEnable(GL_BLEND));
        CHECKED_GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        CHECKED_GL_CALL(glPointSize(24.0f));

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
        shadowProg->addUniform("MatAmb");
        shadowProg->addUniform("MatDif");
        shadowProg->addUniform("MatSpec");
        shadowProg->addUniform("MatShine");
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
        texProg->addUniform("lightColor");
        texProg->addAttribute("vertPos");
        texProg->addAttribute("vertNor");
        texProg->addAttribute("vertTex");

        partProg = make_shared<Program>();
        partProg->setVerbose(true);
        partProg->setShaderNames(
            resourceDirectory + "/particle_vert.glsl",
            resourceDirectory + "/particle_frag.glsl");
        if (!partProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        partProg->addUniform("P");
        partProg->addUniform("M");
        partProg->addUniform("V");
        // partProg->addUniform("pColor");
        partProg->addUniform("alphaTexture");
        partProg->addAttribute("vertPos");
        partProg->addAttribute("vertColor");

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

        texture3 = make_shared<Texture>();
        texture3->setFilename(resourceDirectory + "/alpha.bmp");
        texture3->init();
        texture3->setUnit(3);
        texture3->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

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

        // initialize particle system
        particleSystem = make_shared<particleSys>(vec3(0, 50, 0));
        particleSystem->gpuSetup();
    }

    void initGeom(const std::string &resourceDirectory)
    {
        // EXAMPLE set up to read one shape from one obj file - convert to read several
        //  Initialize mesh
        //  Load geometry
        //  Some obj files contain material information.We'll ignore them for this assignment.
        vector<tinyobj::shape_t> TOshapes;
        string errStr;
        // load in the mesh and make the shape(s)
        bool rc = tinyobj::LoadObj(TOshapes, sphereMaterials, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
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

        treeModel = loadModel("/meshes/tree.obj", resourceDirectory, treeMaterials);
        eeveeModel = loadModel("/meshes/eevee.obj", resourceDirectory, eeveeMaterials);
        dittoModel = loadModel("/meshes/ditto.obj", resourceDirectory, dittoMaterials);
        snorlaxModel = loadModel("/meshes/snorlax.obj", resourceDirectory, snorlaxMaterials);
        bulbasaurModel = loadModel("/meshes/bulbasaur.obj", resourceDirectory, bulbasaurMaterials);
        lucarioModel = loadModel("/meshes/lucario.obj", resourceDirectory, lucarioMaterials);
        mewModel = loadModel("/meshes/mew.obj", resourceDirectory, mewMaterials);
        umbreonModel = loadModel("/meshes/umbreon.obj", resourceDirectory, umbreonMaterials);

        models = {
            treeModel,
            bulbasaurModel,
            dittoModel,
            eeveeModel,
            lucarioModel,
            mewModel,
            snorlaxModel,
            umbreonModel,
        };

        modelMinMaxes.resize(models.size());
        for (int i = 0; i < models.size(); i++)
            modelMinMaxes[i] = ModelUtils::measureModel(models[i]);

        modelNormMats.resize(models.size());
        for (int i = 0; i < models.size(); i++)
            modelNormMats[i] = ModelUtils::getNormalizationMatrix(modelMinMaxes[i].first, modelMinMaxes[i].second);

        // code to load in the ground plane (CPU defined data passed to GPU)
        initGround();
        loadWorld(resourceDirectory + "/world.sav");
        printf("Loaded world grid of size %lu x %lu\n", worldGrid.size(), worldGrid[0].size());
    }

    vector<shared_ptr<Shape>> loadModel(const std::string &filename, const std::string &resourceDirectory, std::vector<tinyobj::material_t> &materialsOut)
    {
        vector<shared_ptr<Shape>> model;
        vector<tinyobj::shape_t> TOshapes;
        string errStr;

        bool rc = tinyobj::LoadObj(TOshapes, materialsOut, errStr, (resourceDirectory + filename).c_str());
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
        SetModelTransforms(vec3(0, -1, 0), 0, 0, 4, curS);
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

    TileType stringToTile(const string &s)
    {
        if (s == "tree")
            return TREE;
        if (s == "bulbasaur")
            return BULBASAUR;
        if (s == "ditto")
            return DITTO;
        if (s == "eevee")
            return EEVEE;
        if (s == "lucario")
            return LUCARIO;
        if (s == "mew")
            return MEW;
        if (s == "snorlax")
            return SNORLAX;
        if (s == "umbreon")
            return UMBREON;

        return GRASS;
    }

    void loadWorld(const string &filename)
    {
        ifstream file(filename);
        string line;

        while (getline(file, line))
        {
            stringstream ss(line);
            vector<TileType> row;
            string cell;

            while (ss >> cell)
                row.push_back(stringToTile(cell));

            worldGrid.push_back(row);
        }

        file.close();
    }

    float pseudoRandom(int x, int z, int salt = 0)
    {
        unsigned int seed = x * 73856093u ^ z * 19349663u ^ salt;
        seed = (seed ^ (seed >> 13)) * 1274126177u;
        return float(seed % 10000) / 10000.0f;
    }

    void drawWorld(shared_ptr<Program> prog, shared_ptr<MatrixStack> &Model)
    {
        for (int z = 0; z < worldGrid.size(); z++)
        {
            for (int x = 0; x < worldGrid[z].size(); x++)
            {
                TileType tile = worldGrid[z][x];

                Model->pushMatrix();
                float centerX = worldGrid[0].size() / 2.0f;
                float centerZ = worldGrid.size() / 2.0f;

                Model->translate(vec3((x - centerX) * tileSize, -2.0f, (z - centerZ) * tileSize));

                // position tile in world
                // Model->translate(vec3(x * tileSize, -1, z * tileSize));
                Model->scale(vec3(0.85f));
                float r = pseudoRandom(x, z);
                float angle = r * 360.0f;
                float scale = 4.0f + r * 18.0f;
                switch (tile)
                {
                case TREE:
                {
                    Model->pushMatrix();

                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));
                    // Model->translate(vec3(0, .0f, 0));
                    Model->scale(vec3(scale));

                    Model->multMatrix(modelNormMats[0]);

                    SetModel(prog, Model);
                    SetMaterial(prog, TREE, vec2(x, z));
                    drawModel(prog, treeModel);

                    Model->popMatrix();
                    break;
                }

                case BULBASAUR:
                {
                    Model->pushMatrix();

                    Model->translate(vec3(0, 2.0f, 0));
                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));
                    Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                    Model->scale(vec3(2.0f));

                    Model->multMatrix(modelNormMats[1]);

                    SetModel(prog, Model);
                    SetMaterial(prog, BULBASAUR, vec2(x, z));
                    drawModel(prog, bulbasaurModel);

                    Model->popMatrix();
                    break;
                }

                case DITTO:
                {
                    Model->pushMatrix();

                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));

                    Model->multMatrix(modelNormMats[2]);

                    SetModel(prog, Model);
                    SetMaterial(prog, DITTO, vec2(x, z));
                    drawModel(prog, dittoModel);

                    Model->popMatrix();
                    break;
                }

                case EEVEE:
                {
                    Model->pushMatrix();

                    Model->translate(vec3(0, 0.8, 0));
                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));
                    Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));

                    Model->multMatrix(modelNormMats[3]);

                    SetModel(prog, Model);
                    SetMaterial(prog, EEVEE, vec2(x, z));
                    drawModel(prog, eeveeModel);

                    Model->popMatrix();
                    break;
                }

                case LUCARIO:
                {
                    Model->pushMatrix();

                    Model->translate(vec3(0, 2.0f, 0));
                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));
                    Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                    Model->scale(vec3(2.0f));

                    Model->multMatrix(modelNormMats[4]);

                    SetModel(prog, Model);
                    SetMaterial(prog, LUCARIO, vec2(x, z));
                    drawModel(prog, lucarioModel);

                    Model->popMatrix();
                    break;
                }

                case MEW:
                {
                    Model->pushMatrix();

                    Model->translate(vec3(0, 4.0f, 0));
                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));
                    Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                    Model->scale(vec3(2.0f));

                    Model->multMatrix(modelNormMats[5]);

                    SetModel(prog, Model);
                    SetMaterial(prog, MEW, vec2(x, z));
                    drawModel(prog, mewModel);

                    Model->popMatrix();
                    break;
                }

                case SNORLAX:
                {
                    Model->pushMatrix();

                    Model->translate(vec3(0, 1.0f, 0));
                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));
                    Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                    Model->scale(vec3(4.0f));

                    Model->multMatrix(modelNormMats[6]);

                    SetModel(prog, Model);
                    SetMaterial(prog, SNORLAX, vec2(x, z));
                    drawModel(prog, snorlaxModel);

                    Model->popMatrix();
                    break;
                }

                case UMBREON:
                {
                    Model->pushMatrix();

                    Model->translate(vec3(0, 1.5f, 0));
                    Model->rotate(glm::radians(angle), vec3(0, 1, 0));
                    Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
                    Model->scale(vec3(2.0f));

                    Model->multMatrix(modelNormMats[7]);

                    SetModel(prog, Model);
                    SetMaterial(prog, UMBREON, vec2(x, z));
                    drawModel(prog, umbreonModel);

                    Model->popMatrix();
                    break;
                }

                default:
                    break;
                }

                Model->popMatrix();
            }
        }
    }

    void SetLight(shared_ptr<Program> curS)
    {
        glUniform3f(curS->getUniform("lightPos"), 0.0f - lightTransX, 0.0f + lightTransY, 0.0f + lightTransZ);
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
        particleSystem->setCamera(Cam);
        glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
    }

    void SetMaterial(shared_ptr<Program> curS, const int materialId, glm::vec2 pos = glm::vec2(0))
    {
        vec3 MatAmb, MatDif, MatSpec;
        float MatShine;

        float rAmb = pseudoRandom(pos.x, pos.y, 1);
        float rDif = pseudoRandom(pos.x, pos.y, 2);
        float rSpec = pseudoRandom(pos.x, pos.y, 3);

        switch (materialId)
        {
        case TREE:
            MatAmb = vec3(0.1f, 0.15f, 0.05f) * (0.1f + rAmb * 0.9f);
            MatDif = vec3(0.1f, 0.3f, 0.1f) * (0.1f + rDif * 0.9f);
            MatSpec = vec3(0.05f) * (0.4f + rSpec * 0.6f);
            MatShine = 16.0f;
            break;
        case BULBASAUR:
            if (toggleMaterial)
            {
                MatAmb = vec3(0.12f, 0.14f, 0.05f);
                MatDif = vec3(0.74f, 0.88f, 0.20f);
                MatSpec = vec3(0.35f, 0.40f, 0.15f);
                MatShine = 32.0f;
            }
            else
            {
                MatAmb = vec3(0.02f, 0.03f, 0.02f) * (0.8f + rAmb * 0.2f);
                MatDif = vec3(0.2f, 0.6f, 0.4f) * (0.8f + rDif * 0.2f);
                MatSpec = vec3(0.15f, 0.2f, 0.15f) * (0.8f + rSpec * 0.2f);
                MatShine = 20.0f;
            }
            break;
        case DITTO:
            if (toggleMaterial)
            {
                MatAmb = vec3(0.06f, 0.09f, 0.12f);
                MatDif = vec3(0.42f, 0.67f, 0.90f);
                MatSpec = vec3(0.40f, 0.50f, 0.60f);
                MatShine = 40.0f;
            }
            else
            {
                MatAmb = vec3(0.03f, 0.02f, 0.04f) * (0.8f + rAmb * 0.2f);
                MatDif = vec3(0.8f, 0.6f, 0.9f) * (0.8f + rDif * 0.2f);
                MatSpec = vec3(0.25f, 0.2f, 0.3f) * (0.8f + rSpec * 0.2f);
                MatShine = 20.0f;
            }
            break;
        case EEVEE:
            if (toggleMaterial)
            {
                MatAmb = vec3(0.03f, 0.025f, 0.015f);
                MatDif = vec3(0.9f, 0.8f, 0.55f);
                MatSpec = vec3(0.25f, 0.22f, 0.18f);
                MatShine = 28.0f;
            }
            else
            {
                MatAmb = vec3(0.03f, 0.02f, 0.01f) * (0.8f + rAmb * 0.2f);
                MatDif = vec3(0.55f, 0.33f, 0.2f) * (0.8f + rDif * 0.2f);
                MatSpec = vec3(0.2f, 0.18f, 0.15f) * (0.8f + rSpec * 0.2f);
                MatShine = 20.0f;
            }
            break;
        case LUCARIO:
            if (toggleMaterial)
            {
                MatAmb = vec3(0.12f, 0.10f, 0.04f);
                MatDif = vec3(0.88f, 0.76f, 0.18f);
                MatSpec = vec3(0.55f, 0.55f, 0.35f);
                MatShine = 64.0f;
            }
            else
            {
                MatAmb = vec3(0.05f, 0.05f, 0.05f) * (0.8f + rAmb * 0.2f);
                MatDif = vec3(0.0f, 0.4f, 0.6f) * (0.8f + rDif * 0.2f);
                MatSpec = vec3(0.6f, 0.6f, 0.65f) * (0.8f + rSpec * 0.2f);
                MatShine = 64.0f;
            }
            break;
        case MEW:
            if (toggleMaterial)
            {
                MatAmb = vec3(0.15f, 0.18f, 0.22f);
                MatDif = vec3(0.60f, 0.78f, 0.92f);
                MatSpec = vec3(0.50f, 0.60f, 0.70f);
                MatShine = 64.0f;
            }
            else
            {
                MatAmb = vec3(0.2f, 0.15f, 0.2f) * (0.8f + rAmb * 0.2f);
                MatDif = vec3(1.0f, 0.8f, 0.9f) * (0.8f + rDif * 0.2f);
                MatSpec = vec3(0.5f, 0.5f, 0.5f) * (0.8f + rSpec * 0.2f);
                MatShine = 64.0f;
            }
            break;
        case SNORLAX:
            if (toggleMaterial)
            {
                MatAmb = vec3(0.02f, 0.04f, 0.06f);
                MatDif = vec3(0.08f, 0.25f, 0.45f);
                MatSpec = vec3(0.25f, 0.30f, 0.35f);
                MatShine = 32.0f;
            }
            else
            {
                MatAmb = vec3(0.02f, 0.02f, 0.03f) * (0.8f + rAmb * 0.2f);
                MatDif = vec3(0.15f, 0.3f, 0.45f) * (0.8f + rDif * 0.2f);
                MatSpec = vec3(0.2f, 0.25f, 0.3f) * (0.8f + rSpec * 0.2f);
                MatShine = 16.0f;
            }
            break;
        case UMBREON:
            if (toggleMaterial)
            {
                MatAmb = vec3(0.05f, 0.05f, 0.08f);
                MatDif = vec3(0.08f, 0.10f, 0.18f);
                MatSpec = vec3(0.30f, 0.45f, 0.80f);
                MatShine = 64.0f;
            }
            else
            {
                MatAmb = vec3(0.05f, 0.05f, 0.05f) * (0.8f + rAmb * 0.2f);
                MatDif = vec3(0.05f, 0.05f, 0.05f) * (0.8f + rDif * 0.2f);
                MatSpec = vec3(0.2f, 0.25f, 0.3f) * (0.8f + rSpec * 0.2f);
                MatShine = 32.0f;
            }
            break;
        default:
            MatAmb = vec3(0.2f) * (0.8f + rAmb * 0.2f);
            MatDif = vec3(0.5f) * (0.8f + rDif * 0.2f);
            MatSpec = vec3(0.3f) * (0.8f + rSpec * 0.2f);
            MatShine = 32.0f;
            break;
        }
        glUniform3f(curS->getUniform("MatAmb"), MatAmb.x, MatAmb.y, MatAmb.z);
        glUniform3f(curS->getUniform("MatDif"), MatDif.x, MatDif.y, MatDif.z);
        glUniform3f(curS->getUniform("MatSpec"), MatSpec.x, MatSpec.y, MatSpec.z);
        glUniform1f(curS->getUniform("MatShine"), MatShine);
    }

    /* code to draw waving hierarchical model */
    void drawHierModel(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog)
    {
        Model->pushMatrix();
        {
            Model->translate(vec3(0, 1.5, -12));

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

    void drawModel(shared_ptr<Program> prog, const vector<shared_ptr<Shape>> &model)
    {
        for (auto &mesh : model)
        {
            mesh->draw(prog);
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
        float speed = 20.0f * frametime;
        eye += strafe * movementInput.x * speed;
        eye += vertical * movementInput.y * speed;
        eye += view * movementInput.z * speed;

        glm::vec3 target;
        target.x = cos(g_phi) * cos(g_theta);
        target.y = sin(g_phi);
        target.z = cos(g_phi) * cos(glm::pi<float>() / 2.0f - g_theta);

        g_eye = eye;
        g_lookAt = g_eye - target;

        // particleSystem->setStart(g_eye);

        // update the camera position
        updateUsingCameraPath(frametime);

        // Apply perspective projection.
        Projection->pushMatrix();
        Projection->perspective(45.0f, aspect, 0.01f, 150.0f);

        texProg->bind();
        glUniform1i(texProg->getUniform("flip"), 1);
        drawGround(texProg);
        texProg->unbind();

        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0, -1.0);

        shadowProg->bind();
        Model->pushMatrix();
        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        SetView(shadowProg);
        SetLight(shadowProg);
        Model->translate(vec3(0.0f, g_groundY - 1.7f, 0.0f));
        Model->scale(vec3(1.0f, 0.0f, 1.0f));
        drawHierModel(Model, shadowProg);
        drawWorld(shadowProg, Model);
        Model->popMatrix();
        shadowProg->unbind();

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_POLYGON_OFFSET_FILL);

        // Draw the doggos
        texProg->bind();
        glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        SetView(texProg);
        SetLight(texProg);
        glUniform1f(texProg->getUniform("MatShine"), 27.9);
        Model->pushMatrix();

        // draw big background sphere
        glUniform1i(texProg->getUniform("flip"), 0);
        texture1->bind(texProg->getUniform("Texture0"));
        Model->pushMatrix();
        Model->loadIdentity();
        Model->scale(vec3(75.0));
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

        texProg->unbind();

        // use the material shader
        // set up all the matrices
        prog->bind();
        glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        SetView(prog);
        SetLight(prog);
        drawWorld(prog, Model);
        prog->unbind();

        partProg->bind();
        texture3->bind(partProg->getUniform("alphaTexture"));
        SetView(partProg);
        // SetLight(partProg);
        CHECKED_GL_CALL(glUniformMatrix4fv(partProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix())));
        // CHECKED_GL_CALL(glUniformMatrix4fv(partProg->getUniform("V"), 1, GL_FALSE, value_ptr(View->topMatrix())));
        CHECKED_GL_CALL(glUniformMatrix4fv(partProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix())));

        // CHECKED_GL_CALL(glUniform3f(partProg->getUniform("pColor"), 0.9, 0.7, 0.7));
        particleSystem->drawMe(partProg);
        particleSystem->update(frametime);
        partProg->unbind();

        // shadowProg->bind();
        // glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
        // SetView(shadowProg);
        // SetLight(shadowProg);
        // glUniform3f(shadowProg->getUniform("lightPos"), 2.0 + lightTrans, 2.0, 2.9);

        // const float epsilon = 0.001f;

        // Model->pushMatrix();
        // {
        //     Model->translate(vec3(0.0f, g_groundY + epsilon - 1.0f, 0.0f));
        //     Model->scale(vec3(1.0f, 0.0f, 1.0f));
        //     drawHierModel(Model, shadowProg);
        // }
        // Model->popMatrix();

        // for (int i = 0; i < 4; i++)
        // {
        //     for (int j = 0; j < 4; j++)
        //     {
        //         int idx = (i + j) % 4;
        //         Model->pushMatrix();
        //         {
        //             Model->translate(vec3(0.0f, g_groundY + epsilon - 1.0f, 0.0f));
        //             Model->scale(vec3(1.0f, 0.0f, 1.0f));
        //             Model->translate(vec3(off + sp * i, -1, off + sp * j));
        //             Model->scale(vec3(0.85f));
        //             Model->rotate(glm::radians(-30.0f * i), vec3(0, 1, 0));
        //             auto model = models[idx];
        //             Model->pushMatrix();
        //             {
        //                 if (idx == 0 || idx == 3)
        //                 {
        //                     Model->translate(vec3(0, 1, 0));
        //                     Model->rotate(glm::radians(-90.0f), vec3(1, 0, 0));
        //                 }
        //                 if (idx == 2)
        //                 {
        //                     Model->scale(vec3(2.0f, 2.0f, 2.0f));
        //                 }
        //                 Model->multMatrix(ModelUtils::getNormalizationMatrix(modelMinMaxes[idx].first, modelMinMaxes[idx].second));
        //                 glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
        //                 drawModel(shadowProg, model);
        //             }
        //             Model->popMatrix();
        //         }
        //         Model->popMatrix();
        //     }
        // }

        // shadowProg->unbind();

        // update animation data
        animationTheta = sin(glfwGetTime());
        // movementInput = glm::vec3(0.0f);

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
