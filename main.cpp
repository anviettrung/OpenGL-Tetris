#pragma region Includes
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <vector>

using namespace std;
using namespace glm;

#pragma endregion

#pragma region Settings

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

const int mWidth = 9;
const int mHeight = 16;
const int totalCell = mWidth * mHeight;
vec3 cameraPos = vec3(4.5f, 8, 0);
const double tickTime = 1.f;

#pragma region Random

random_device dev;
mt19937 rng(dev());
uniform_int_distribution<std::mt19937::result_type> dist7(0, 7);

#pragma endregion

#pragma endregion

#pragma region Shader

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform vec3 tint;\n"
"out vec3 ourColor;\n"
"uniform mat4 transform;\n"
"void main()\n"
"{\n"
"   gl_Position = transform * vec4(aPos, 1.0);\n"
"   ourColor = tint / 255;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(ourColor, 1.0f);\n"
"}\n\0";

#pragma endregion

#pragma region Data Stucture

struct Vec2Int {
    int x, y;
    Vec2Int(int _x, int _y) : x(_x), y(_y) {}
    Vec2Int() : x(0), y(0) {}
};

struct Transform {
    Vec2Int position;
    int rotation;

    Transform(Vec2Int pos, int rot) : position(pos), rotation(rot) {}
    Transform() : position(Vec2Int()), rotation(0) {}
};

struct Color { 
    float data[3];

    Color(float r, float g, float b) 
    {
        data[0] = r;
        data[1] = g;
        data[2] = b;
    }

    Color() {
        for (int i = 0; i < 3; i++)
            data[i] = 0;
    }
};

struct Block {
    Vec2Int data[4];
    Color* color;
    
    Block(Vec2Int p[4], Color* c)
    {
        for (int i = 0; i < 4; i++)
            data[i] = p[i];
        color = c;
    }

    Block(int p[8], Color* c)
    {
        for (int i = 0; i < 4; i++)
        {
            data[i].x = p[2 * i];
            data[i].y = p[2 * i + 1];
        }
        color = c;
    }
};

struct Tetromino {
    Transform transform;
    Block* block;

    Tetromino(Transform trans, Block* b) {
        transform = trans;
        block = b;
    }
};

#pragma endregion

#pragma region Asset

unsigned int VBO, VAO, EBO;

float mesh_vertex_square[] = {
    -0.5f,  0.5f, 0.0f, // top left
    -0.5f, -0.5f, 0.0f, // bot left
     0.5f, -0.5f, 0.0f, // bot right
     0.5f,  0.5f, 0.0f  // top right
};

unsigned int mesh_index_square[] = {
    0, 1, 2,
    0, 2, 3
};

Color color_black = Color(0, 0, 0);
Color color_white = Color(255, 255, 255);
Color color_grey = Color(225, 225, 225);
Color color_dgrey = Color(128, 128, 128);
Color color_cyan = Color(0, 183, 235);
Color color_blue = Color(0, 0, 255);
Color color_orange = Color(255, 128, 0);
Color color_yellow = Color(255, 255, 0);
Color color_green = Color(0, 255, 0);
Color color_purple = Color(255, 0, 255);
Color color_red = Color(255, 0, 0);

Block blocks[] =
{
    // Block I
    Block(new int[]{
        1,  2,
        1,  1,
        1,  0,
        1, -1
    }, &color_cyan),
    // Block L1
    Block(new int[] {
        1, 2,
        1, 1,
        1, 0,
        0, 0
    }, &color_blue),
    // Block L2
    Block(new int[] {
        0, 2,
        0, 1,
        0, 0,
        1, 0
    }, &color_orange),
    // Block O
    Block(new int[] {
        0, 1,
        1, 1,
        0, 0,
        1, 0
    }, &color_yellow),
    // Block Z1
    Block(new int[] {
        1, 2,
        1, 1,
        0, 1,
        0, 0
    }, &color_green),
    // Block Z2
    Block(new int[] {
        0, 2,
        0, 1,
        1, 1,
        1, 0
    }, &color_red),
    // Block T
    Block(new int[] {
        0, 2,
        0, 1,
        1, 1,
        0, 0
    }, &color_purple)
};

map<int, int> track_key_state = {
    { GLFW_KEY_UP, GLFW_RELEASE },
    { GLFW_KEY_DOWN, GLFW_RELEASE},
    { GLFW_KEY_LEFT, GLFW_RELEASE },
    { GLFW_KEY_RIGHT, GLFW_RELEASE },
    { GLFW_KEY_E, GLFW_RELEASE },
    { GLFW_KEY_W, GLFW_RELEASE }
};

#pragma endregion

#pragma region Declaration

// Input
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput();
void updateTrackKeyInput();
bool IsKeyDown(int key);
bool IsKeyUp(int key);

// Render
template <size_t n_vert, size_t n_index>
void binding(float(&vertices)[n_vert], unsigned int(&indices)[n_index]);
void clearColor(Color& color);

// Core Game
void ProcessPlayerControl();
void TickDown();
void Update();
void OnLanded();
void CheckClearTetromino();

// Tetromino
void ClearTetromino(Tetromino* tetro);
void DrawTetromino(Tetromino* tetro);
bool CheckTetrominoTransform(Tetromino* tetro);

// Utilities
Vec2Int ApplyRotate(Vec2Int pos, int rot);

#pragma endregion

#pragma region Working var

GLFWwindow* window;
double passTickTime = 0;
const Vec2Int startPos = Vec2Int(mWidth / 2, mHeight);
float* cell[totalCell];
int cTetroId = 3;
Transform oldTrans;
Tetromino cTetro = Tetromino(
    Transform(Vec2Int(3, 4), 0),
    &blocks[cTetroId]
);
vector<int> scanLines;

#pragma endregion

int main()
{
#pragma region Initialization

#pragma region Init GLFW

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

#pragma endregion

#pragma region Init Window

    // glfw window creation
    // --------------------
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

#pragma endregion

#pragma region Init GLAD

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

#pragma endregion

#pragma region Shader Compile

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

#pragma endregion

#pragma region Binding

    binding(mesh_vertex_square, mesh_index_square);
    glBindVertexArray(VAO);
    glUseProgram(shaderProgram);

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "tint");

    for (int i = 0; i < totalCell; i++)
        cell[i] = color_grey.data;

#pragma endregion

#pragma endregion

#pragma region Core Loop

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        ClearTetromino(&cTetro);

        processInput();
        Update();
        updateTrackKeyInput();

        DrawTetromino(&cTetro);

        // render
        #pragma region Render

        clearColor(color_white);
        glClear(GL_COLOR_BUFFER_BIT);

        // Border
        {
            mat4 trans = mat4(1.0f);
            trans = scale(trans, 0.12f * vec3(mWidth, mHeight, 1));
            trans = translate(trans, vec3(mWidth / 2, mHeight / 2, 0) - cameraPos + vec3(0.49f, 0.015f, 0.0f));
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
            glUniform3fv(colorLoc, 1, color_dgrey.data);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // Cells
        for (int i = 0; i < totalCell; i++)
        {
            int pos_x = i % mWidth;
            int pos_y = i / mWidth;

            mat4 trans = mat4(1.0f);
            trans = scale(trans, vec3(1.0f) * 0.1f);
            trans = translate(trans, 1.1f * vec3(pos_x, pos_y, 0) - cameraPos);

            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
            glUniform3fv(colorLoc, 1, cell[i]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        #pragma endregion

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

#pragma endregion

#pragma region Destruction

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;

#pragma endregion

}

#pragma region Implementation

#pragma region Input

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void updateTrackKeyInput()
{
    for (map<int, int>::iterator i = track_key_state.begin(); i != track_key_state.end(); i++)
        i->second = glfwGetKey(window, i->first);
}

bool IsKeyDown(int key) {
    return track_key_state[key] == GLFW_RELEASE && glfwGetKey(window, key) == GLFW_PRESS;
}

bool IsKeyUp(int key)
{
    return track_key_state[key] == GLFW_PRESS && glfwGetKey(window, key) == GLFW_RELEASE;
}

#pragma endregion

#pragma region Render

template <size_t n_vert, size_t n_index>
void binding(float(&vertices)[n_vert], unsigned int(&indices)[n_index])
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n_vert, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * n_index, indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    // glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void clearColor(Color& color)
{
    glClearColor(color.data[0], color.data[1], color.data[2], 1.0);
}

#pragma endregion

#pragma region Core Game

void Update()
{
    ProcessPlayerControl();
    TickDown();
}

void ProcessPlayerControl()
{
    oldTrans = cTetro.transform;

    if (IsKeyDown(GLFW_KEY_E))
    {
        cTetroId = (cTetroId + 1) % 7;
        cTetro.block = &blocks[cTetroId];
    }

    if (IsKeyDown(GLFW_KEY_W))
        cTetro.transform.rotation = (cTetro.transform.rotation + 1) % 4;

    if (IsKeyDown(GLFW_KEY_LEFT))
        cTetro.transform.position.x -= 1;

    if (IsKeyDown(GLFW_KEY_RIGHT))
        cTetro.transform.position.x += 1;

    if (IsKeyDown(GLFW_KEY_DOWN))
        cTetro.transform.position.y -= 1;

    if (IsKeyDown(GLFW_KEY_UP))
        cTetro.transform.position.y += 1;

    if (!CheckTetrominoTransform(&cTetro))
        cTetro.transform = oldTrans;
}


void TickDown()
{
    while (passTickTime + tickTime < glfwGetTime())
    {
        passTickTime += tickTime;

        oldTrans = cTetro.transform;

        cTetro.transform.position.y -= 1;

        if (!CheckTetrominoTransform(&cTetro))
        {
            cTetro.transform = oldTrans;
            OnLanded();
        }
    }
}

void OnLanded()
{
    DrawTetromino(&cTetro);
    cTetro.block = &blocks[dist7(rng)];
    cTetro.transform.position = startPos;
    cTetro.transform.rotation = 0;
}

void CheckClearTetromino()
{
    int countFill = 0;
    int stopLine = 0;
    for (int y = 0; y < mHeight; y++)
    {
        countFill = 0;
        for (int x = 0; x < mWidth; x++)
            if (cell[y * mWidth + x] == color_grey.data)
                countFill++;

        if (countFill == mWidth)
        {
            stopLine = y;
            break;
        }

        if (countFill == 0)
            scanLines.push_back(y);
    }


    int sLineIndex = 0;
    for (int y = 0; y < stopLine; y++)
    {
        if (sLineIndex > 0)
            for (int x = 0; x < mWidth; x++)
                cell[y * mWidth + x] == cell[(y + sLineIndex) * mWidth + x];

        if (y + 1 == scanLines[sLineIndex])
            sLineIndex++;
    }

    for (int i = 0; i < scanLines.size(); i++)
    {
        for (int x = 0; x < mWidth; x++)
            cell[scanLines[i] * mWidth + x] == cell[(scanLines[i] + i + 1) * mWidth + x];
    }
    scanLines.clear();
}

#pragma endregion

#pragma region Tetromino

bool CheckTetrominoTransform(Tetromino* tetro)
{
    for (int i = 0; i < 4; i++)
    {
        Vec2Int local_pos = tetro->block->data[i];
        local_pos = ApplyRotate(local_pos, tetro->transform.rotation);
        int px = tetro->transform.position.x + local_pos.x;
        int py = tetro->transform.position.y + local_pos.y;
        if (0 <= px && px < mWidth && 0 <= py && py < mHeight)
        {
            if (cell[py * mWidth + px] != color_grey.data)
                return false;
        }
        else {
            if (px < 0 || px >= mWidth || py < 0)
                return false;
        }
    }

    return true;
}

void ClearTetromino(Tetromino* tetro)
{
    for (int i = 0; i < 4; i++)
    {
        Vec2Int local_pos = tetro->block->data[i];
        local_pos = ApplyRotate(local_pos, tetro->transform.rotation);
        int px = tetro->transform.position.x + local_pos.x;
        int py = tetro->transform.position.y + local_pos.y;
        if (px < 0 || px >= mWidth || py < 0 || py >= mHeight)
            continue;
        cell[py * mWidth + px] = color_grey.data;
    }
}

void DrawTetromino(Tetromino* tetro)
{
    for (int i = 0; i < 4; i++)
    {
        Vec2Int local_pos = tetro->block->data[i];
        local_pos = ApplyRotate(local_pos, tetro->transform.rotation);
        int px = tetro->transform.position.x + local_pos.x;
        int py = tetro->transform.position.y + local_pos.y;
        if (px < 0 || px >= mWidth || py < 0 || py >= mHeight)
            continue;
        cell[py * mWidth + px] = tetro->block->color->data;
    }
}

#pragma endregion

#pragma region Utilities

Vec2Int ApplyRotate(Vec2Int pos, int rot)
{
    int t = pos.x;
    switch (rot)
    {
    case 1: // 90 deg
        pos.x = pos.y;
        pos.y = 1 - t;
        break;
    case 2: // 180 deg
        pos.x = 1 - pos.x;
        pos.y = 1 - pos.y;
        break;
    case 3: // 270 deg
        pos.x = 1 - pos.y;
        pos.y = t;
        break;
    }

    return pos;
}

#pragma endregion

#pragma endregion

