#include <iostream>
#include <map>
#include <vector>
#include <random>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

#pragma region Settings

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

const int w = 9;
const int h = 16;
const double tick = 1;

#pragma endregion

// Tham khảo Shader của An Việt Trung
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

#pragma region Asset

map<int, int> track_key_state = {
    { GLFW_KEY_UP, GLFW_RELEASE },
    { GLFW_KEY_DOWN, GLFW_RELEASE},
    { GLFW_KEY_LEFT, GLFW_RELEASE },
    { GLFW_KEY_RIGHT, GLFW_RELEASE },
    { GLFW_KEY_E, GLFW_RELEASE },
    { GLFW_KEY_W, GLFW_RELEASE }
};

// Danh sách màu
float color_background[] = {  87, 155, 177 };
float color_cell_empty[] = { 225, 215, 198 };
float color_begie[] = { 251, 194, 82 };
float color_sage[] = { 163, 187, 152 };

float* colors[] = {
    color_cell_empty,
    color_begie,
    color_sage
};

int blocks[7][4][4] = {
    {
        { 2, 6, 10, 14 },
        { 8, 9, 10, 11 },
        { 1, 5, 9, 13 },
        { 4, 5, 6, 7 }
    },
    {
        { 5, 6, 9, 10 },
        { 5, 6, 9, 10 },
        { 5, 6, 9, 10 },
        { 5, 6, 9, 10 }
    }
};

#pragma endregion

#pragma region Declaration

// Input
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput();
void updateTrackKeyInput();
bool IsKeyDown(int key);
bool IsKeyUp(int key);

// Core Game
void Control();
void Update();

void InitBrick();
void Fall();
void StopBrick();
void DeleteRow();

bool Check(int x, int y, int block_id, int quay_id);
void Draw(int x, int y, int block_id, int quay_id, int color_id);

void CopyBrick();
void Apply();

#pragma endregion

#pragma region Global Variables

GLFWwindow* window;
double passTickTime = 0;

int matrix[w][h];

int BLOCK = 0;
int X = 0;
int Y = 0;
int quay = 0;

int m_BLOCK = BLOCK;
int m_X = X;
int m_Y = Y;
int m_quay = quay;

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

#pragma endregion

#pragma region VAO, VBO, EBO

    float vertices[] = {
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindVertexArray(VAO);
    glUseProgram(shaderProgram);

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "tint");

#pragma endregion

#pragma region Core Loop

    InitBrick();

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        processInput();
        Update();
        updateTrackKeyInput();

        #pragma region Render

        glClearColor(color_background[0] / 255, color_background[1] / 255, color_background[2] / 255, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        for (int x = 0; x < w; x++)
        {
            for (int y = 0; y < h; y++)
            {
                mat4 trans = mat4(1.0f);
                trans = scale(trans, vec3(1.0f) * 0.1f);
                trans = translate(trans, 1.1f * vec3(x, y, 0) - vec3(4.5, 8, 0));
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
                glUniform3fv(colorLoc, 1, colors[matrix[x][y]]);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }

        #pragma endregion

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

#pragma endregion

#pragma region Destruction

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;

#pragma endregion

}

#pragma region Implementation

// Tham khảo của An Việt Trung
#pragma region Input

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

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

#pragma region Core Game

void Update()
{
    Draw(X, Y, BLOCK, quay, 0);
    Control();
    Fall();
    Draw(X, Y, BLOCK, quay, 1);
}

void Control()
{
    CopyBrick();

    if (IsKeyDown(GLFW_KEY_E))
        m_BLOCK = (m_BLOCK + 1) % 2;

    if (IsKeyDown(GLFW_KEY_W))
        m_Y++;

    if (IsKeyDown(GLFW_KEY_LEFT))
        m_X--;

    if (IsKeyDown(GLFW_KEY_RIGHT))
        m_X++;

    if (IsKeyDown(GLFW_KEY_DOWN))
        m_Y--;

    if (IsKeyDown(GLFW_KEY_UP))
    {
        m_quay++;
        if (m_quay == 4)
            m_quay = 0;
    }

    if (Check(m_X, m_Y, m_BLOCK, m_quay))
        Apply();
}

void InitBrick()
{
    BLOCK = rand() % 2;
    X = w / 2;
    Y = h - 1;
    quay = 0;
}

void Fall()
{
    while (passTickTime + tick < glfwGetTime())
    {
        passTickTime += tick;

        CopyBrick();

        m_Y--;

        if (Check(m_X, m_Y, m_BLOCK, m_quay))
            Apply();
        else
            StopBrick();
    }
}

void StopBrick()
{
    Draw(X, Y, BLOCK, quay, 1);
    //CheckClearTetromino();
    InitBrick();
}

void DeleteRow()
{
    //int countEmpty = 0;
    //int stopLine = 0;
    //for (int y = 0; y < h; y++)
    //{
    //    countEmpty = 0;
    //    for (int x = 0; x < w; x++)
    //        if (cell[y * w + x] == color_grey.data)
    //            countEmpty++;
    //    
    //    if (countEmpty == w)
    //    {
    //        stopLine = y + 1;
    //        break;
    //    }

    //    if (countEmpty == 0)
    //        scanLines.push_back(y);
    //}
    //
    //int sLineIndex = 0;
    //for (int y = 0; y < stopLine; y++)
    //{
    //    while (sLineIndex < scanLines.size())
    //    {
    //        if (y + sLineIndex != scanLines[sLineIndex]) 
    //            break;
    //    
    //        sLineIndex++;
    //    }

    //    if (sLineIndex > 0)
    //        for (int x = 0; x < w; x++)
    //            cell[y * w + x] = cell[(y + sLineIndex) * w + x];
    //}

    //scanLines.clear();
}

#pragma endregion

bool Check(int x, int y, int block_id, int quay_id)
{
    int* block = blocks[block_id][quay_id];

    for (int i = 0; i < 4; i++)
    {
        int tx = x + block[i] % 4;
        int ty = y + block[i] / 4;

        if (tx < 0 || tx >= w || ty < 0 || matrix[tx][ty] != 0)
            return false;
    }
    return true;
}

void Draw(int x, int y, int block_id, int quay_id, int color_id)
{
    int* block = blocks[block_id][quay_id];

    for (int i = 0; i < 4; i++)
    {
        int tx = x + block[i] % 4;
        int ty = y + block[i] / 4;

        if (0 <= tx && tx < w && 0 <= ty && ty < h)
            matrix[tx][ty] = color_id;
    }
}

void CopyBrick()
{
    m_BLOCK = BLOCK;
    m_X = X;
    m_Y = Y;
    m_quay = quay;
}

void Apply()
{
    BLOCK = m_BLOCK;
    X = m_X;
    Y = m_Y;
    quay = m_quay;
}

#pragma endregion

