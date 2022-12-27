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
const double tick = 0.5f;

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
float color_cell_fill[] = { 251, 194, 82 };

float* colors[] = {
    color_cell_empty,
    color_cell_fill
};

int bricks[7][4][4] = {
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
    },
    {
        { 2, 6, 9, 10 },
        { 5, 9, 10, 11 },
        { 5, 6, 9, 13 },
        { 4, 5, 6, 10 }
    },
    {
        { 1, 5, 9, 10 },
        { 5, 6, 7, 9 },
        { 5, 6, 10, 14 },
        { 6, 8, 9, 10 }
    },
    {
        { 2, 5, 6, 9 },
        { 5, 6, 10, 11 },
        { 6, 9, 10, 13 },
        { 4, 5, 9, 10 }
    },
    {
        { 1, 5, 6, 9 },
        { 5, 6, 7, 10 },
        { 6, 9, 10, 14 },
        { 5, 8, 9, 10 }
    },
    {
        { 1, 5, 6, 10 },
        { 6, 7, 9, 10 },
        { 5, 9, 10, 14 },
        { 5, 6, 8, 9 }
    }
};

#pragma endregion

#pragma region Declaration

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput();
void updateTrackKeyInput();
bool IsKeyDown(int key);
bool IsKeyUp(int key);

void Control(); // Điều khiển
void Update();  // Cập nhật

void InitBrick(); // Khởi tạo ngẫu nhiên Brick mới
void Fall(); // Rơi tự do
void StopBrick(); // Dừng Brick khi chạm đáy
void DeleteRow(); // Xóa hàng

bool Check(int x, int y, int block_id, int quay_id); // Kiểm tra xem brick có hợp lệ
void Draw(int x, int y, int block_id, int quay_id, int color_id); // vẽ brick

void CopyBrick(); // Bản sao biến điều khiển copy bản gốc
void Apply(); // Gán giá trị bản sao biến điều khiển vào bản gốc

#pragma endregion

#pragma region Global Variables

GLFWwindow* window;
double tick_time = 0;

int matrix[w][h];

// Biến điều khiển
int BRICK = 0;
int X = 0;
int Y = 0;
int ROTATE = 0;

// Bản sao biến điều khiển
int m_BRICK = BRICK;
int m_X = X;
int m_Y = Y;
int m_ROTATE = ROTATE;

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

#pragma endregion

#pragma region Core Loop

    InitBrick(); // Khởi tạo một khối khi bắt đầu game

    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        processInput();
        Update();
        updateTrackKeyInput();

        #pragma region Render

        glClearColor(color_background[0] / 255, color_background[1] / 255, color_background[2] / 255, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render ma trận chơi
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
    Draw(X, Y, BRICK, ROTATE, 0);
    Control();
    Fall();
    Draw(X, Y, BRICK, ROTATE, 1);
}

void Control()
{
    CopyBrick();

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
        m_ROTATE--;
        if (m_ROTATE == -1)
            m_ROTATE = 3;
    }

    if (Check(m_X, m_Y, m_BRICK, m_ROTATE))
        Apply();
}

void InitBrick()
{
    BRICK = rand() % 7;
    X = w / 2;
    Y = h - 1;
    ROTATE = 0;
}

void Fall()
{
    while (tick_time + tick < glfwGetTime())
    {
        tick_time += tick;

        CopyBrick();

        m_Y--;

        if (Check(m_X, m_Y, m_BRICK, m_ROTATE))
            Apply();
        else
            StopBrick();
    }
}

void StopBrick()
{
    Draw(X, Y, BRICK, ROTATE, 1);
    DeleteRow();
    InitBrick();
}

void DeleteRow()
{
    int y = 0;
    int empty = 0;
    while (empty < w && y < h) // Hàng trống hoặc hàng trên cùng thì dừng
    {
        empty = 0;
        // Đếm số ô trống
        for (int x = 0; x < w; x++)
            if (matrix[x][y] == 0)
                empty++;

        if (empty == 0) // Hàng kín thì đẩy tất cả hàng trên xuống một đơn vị
            for (int t = y; t < h - 1; t++)
                for (int x = 0; x < w; x++)
                    matrix[x][t] = matrix[x][t + 1];
        else
            y++;
    }
}

bool Check(int x, int y, int block_id, int quay_id)
{
    int* block = bricks[block_id][quay_id];

    for (int i = 0; i < 4; i++)
    {
        int tx = x + block[i] % 4;
        int ty = y + block[i] / 4;

        if (0 <= tx && tx < w && 0 <= ty && ty < h)
        {
            if (matrix[tx][ty] != 0)
                return false;
        }
        else {
            if (tx < 0 || tx >= w || ty < 0)
                return false;
        }
    }
    return true;
}

void Draw(int x, int y, int block_id, int quay_id, int color_id)
{
    int* block = bricks[block_id][quay_id];

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
    m_BRICK = BRICK;
    m_X = X;
    m_Y = Y;
    m_ROTATE = ROTATE;
}

void Apply()
{
    BRICK = m_BRICK;
    X = m_X;
    Y = m_Y;
    ROTATE = m_ROTATE;
}

#pragma endregion

#pragma endregion

