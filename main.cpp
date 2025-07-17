#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "marching_cubes_tables.h"

using namespace std;

// ================== MENÚ DE MÁSCARAS Y COLORES ======================
vector<string> mascaras = {
    "bloodMasks",       // Máscaras de sangre  
    "BrainMasks",       // Máscaras de cerebro  
    "duodenumMasks",  // Máscaras de duodeno  
    "eyeMasks",       // Máscaras de ojo  
    "eyeRetnaMasks",  // Máscaras de retina del ojo  
    "eyeWhiteMasks",  // Máscaras de esclerótica (parte blanca del ojo)  
    "heartMasks",     // Máscaras de corazón  
    "ileumMasks",     // Máscaras de íleon (parte del intestino delgado)  
    "kidneyMasks",    // Máscaras de riñón   
    "lIntestineMasks",// Máscaras de intestino grueso  
    "liverMasks",     // Máscaras de hígado  
    "lungMasks",      // Máscaras de pulmón  
    "muscleMasks",    // Máscaras de músculo  
    "nerveMasks",     // Máscaras de nervio  
    "skeletonMasks",  // Máscaras de esqueleto  
    "spleenMasks",    // Máscaras de bazo  
    "stomachMasks"    // Máscaras de estómago
};

glm::vec3 mascara_colors[] = {
    {0.95, 0.3, 0.5},     // bloodMasks: Sangre - rojo rosado fuerte
    {0.8, 0.9, 1.0},      // brainMasks: Cerebro - azul muy claro (relajante, suave)
    {1.0, 0.8, 0.6},      // duodenumMasks: Duodeno - durazno claro
    {1.0, 1.0, 1.0},     // eyeMasks: Ojo - blanco azulado (más visible que blanco puro)
    {1.0, 0.6, 0.8},      // eyeRetinaMasks: Retina - rosa pastel
    {0.95, 0.95, 0.95},   // eyeWhiteMasks: Esclerótica - gris claro muy pálido (más visible que blanco puro)
    {1.0, 0.3, 0.3},      // heartMasks: Corazón - rojo brillante suave
    {0.9, 0.75, 0.6},     // ileumMasks: Íleon - beige durazno
    {0.7, 0.4, 0.5},      // kidneyMasks: Riñón - vino suave
    {0.6, 0.8, 1.0},      // lIntestineMasks: Intestino grueso - celeste pastel
    {1.0, 0.6, 0.7},      // liverMasks: Hígado - rosado rojizo
    {0.6, 0.9, 0.9},      // lungMasks: Pulmón - aguamarina pastel
    {0.9, 0.7, 0.5},      // muscleMasks: Músculo - salmón claro
    {0.9, 0.85, 0.5},     // nerveMasks: Nervio - amarillo pastel
    {0.96, 0.92, 0.85},   // skeletonMasks: Hueso - marfil
    {0.85, 0.4, 0.5},     // spleenMasks: Bazo - rojo vino pastel
    {1.0, 0.5, 0.3},      // stomachMasks: Estómago - naranja salmón
};

vector<bool> mascara_activa(mascaras.size(), true);

void printMascaraStatus() {
    cout << "\n======= Estado de las máscaras =======" << endl;
    for (size_t i = 0; i < mascara_activa.size(); ++i)
        cout << "[" << (char)((i<9)?('1'+i):('a'+i-9)) << "] "
             << mascaras[i] << ": " << (mascara_activa[i] ? "ON" : "OFF") << endl;
    cout << "[r] Recargar malla con las máscaras actuales" << endl;
    cout << "======================================" << endl << endl;
}

// ================== VARIABLES DE MOVIMIENTO ======================
bool mousePressed = false;
double lastX = 0.0, lastY = 0.0;
float yaw = 0.0f, pitch = 0.0f;
float translateX = 0.0f, translateY = 0.0f;
float zoom = 1.0f;

// ================== CALLBACKS DE INTERACTIVIDAD ==================
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE) {
            mousePressed = false;
        }
    }
}
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mousePressed) {
        float sensitivity = 0.2f;
        float dx = xpos - lastX;
        float dy = ypos - lastY;
        yaw += dx * sensitivity;
        pitch += dy * sensitivity;
        lastX = xpos;
        lastY = ypos;
    }
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoom += yoffset * 0.1f;
    zoom = std::max(0.1f, zoom);
}

// ================== ESTRUCTURAS ======================
struct Punto3D {
    float x, y, z;
    glm::vec3 color;
};
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
};

// ================== SHADERS ======================
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * Color;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * Color;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);  

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";

GLuint crearShaderProgram() {
    GLint success;
    char infoLog[512];
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cerr << "Error compilando vertex shader:\n" << infoLog << endl;
    }
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cerr << "Error compilando fragment shader:\n" << infoLog << endl;
    }
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cerr << "Error al linkear el shader program:\n" << infoLog << endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

glm::vec3 VertexInterp(float isoLevel, glm::vec3 p1, glm::vec3 p2, float valp1, float valp2) {
    if (abs(isoLevel - valp1) < 0.00001) return p1;
    if (abs(isoLevel - valp2) < 0.00001) return p2;
    if (abs(valp1 - valp2) < 0.00001) return p1;
    float mu = (isoLevel - valp1) / (valp2 - valp1);
    return p1 + mu * (p2 - p1);
}
glm::vec3 ColorInterp(float isoLevel, glm::vec3 c1, glm::vec3 c2, float valp1, float valp2) {
    if (abs(isoLevel - valp1) < 0.00001) return c1;
    if (abs(isoLevel - valp2) < 0.00001) return c2;
    if (abs(valp1 - valp2) < 0.00001) return c1;
    float mu = (isoLevel - valp1) / (valp2 - valp1);
    return c1 + mu * (c2 - c1);
}
void MarchingCubes(const vector<vector<vector<uint8_t>>>& volumen,
                   const vector<vector<vector<glm::vec3>>>& volumen_color,
                   vector<Vertex>& vertices,
                   vector<unsigned int>& indices,
                   float isoLevel = 0.9f)
{
    int width = volumen[0][0].size();
    int height = volumen[0].size();
    int depth = volumen.size();
    glm::vec3 vertexList[12];
    glm::vec3 colorList[12];
    for (int z = 0; z < depth - 1; ++z) {
        for (int y = 0; y < height - 1; ++y) {
            for (int x = 0; x < width - 1; ++x) {
                float cubeVal[8];
                glm::vec3 cubePos[8];
                glm::vec3 cubeColor[8];
                for (int i = 0; i < 8; ++i) {
                    int dx = i & 1;
                    int dy = (i & 2) >> 1;
                    int dz = (i & 4) >> 2;
                    cubeVal[i] = volumen[z + dz][y + dy][x + dx];
                    cubePos[i] = glm::vec3(x + dx, y + dy, z + dz);
                    cubeColor[i] = volumen_color[z + dz][y + dy][x + dx];
                }
                int cubeIndex = 0;
                for (int i = 0; i < 8; ++i)
                    if (cubeVal[i] > isoLevel) cubeIndex |= (1 << i);
                int edges = edgeTable[cubeIndex];
                if (edges == 0) continue;
                if (edges & 1) {
                    vertexList[0] = VertexInterp(isoLevel, cubePos[0], cubePos[1], cubeVal[0], cubeVal[1]);
                    colorList[0]  = ColorInterp(isoLevel, cubeColor[0], cubeColor[1], cubeVal[0], cubeVal[1]);
                }
                if (edges & 2) {
                    vertexList[1] = VertexInterp(isoLevel, cubePos[1], cubePos[2], cubeVal[1], cubeVal[2]);
                    colorList[1]  = ColorInterp(isoLevel, cubeColor[1], cubeColor[2], cubeVal[1], cubeVal[2]);
                }
                if (edges & 4) {
                    vertexList[2] = VertexInterp(isoLevel, cubePos[2], cubePos[3], cubeVal[2], cubeVal[3]);
                    colorList[2]  = ColorInterp(isoLevel, cubeColor[2], cubeColor[3], cubeVal[2], cubeVal[3]);
                }
                if (edges & 8) {
                    vertexList[3] = VertexInterp(isoLevel, cubePos[3], cubePos[0], cubeVal[3], cubeVal[0]);
                    colorList[3]  = ColorInterp(isoLevel, cubeColor[3], cubeColor[0], cubeVal[3], cubeVal[0]);
                }
                if (edges & 16) {
                    vertexList[4] = VertexInterp(isoLevel, cubePos[4], cubePos[5], cubeVal[4], cubeVal[5]);
                    colorList[4]  = ColorInterp(isoLevel, cubeColor[4], cubeColor[5], cubeVal[4], cubeVal[5]);
                }
                if (edges & 32) {
                    vertexList[5] = VertexInterp(isoLevel, cubePos[5], cubePos[6], cubeVal[5], cubeVal[6]);
                    colorList[5]  = ColorInterp(isoLevel, cubeColor[5], cubeColor[6], cubeVal[5], cubeVal[6]);
                }
                if (edges & 64) {
                    vertexList[6] = VertexInterp(isoLevel, cubePos[6], cubePos[7], cubeVal[6], cubeVal[7]);
                    colorList[6]  = ColorInterp(isoLevel, cubeColor[6], cubeColor[7], cubeVal[6], cubeVal[7]);
                }
                if (edges & 128) {
                    vertexList[7] = VertexInterp(isoLevel, cubePos[7], cubePos[4], cubeVal[7], cubeVal[4]);
                    colorList[7]  = ColorInterp(isoLevel, cubeColor[7], cubeColor[4], cubeVal[7], cubeVal[4]);
                }
                if (edges & 256) {
                    vertexList[8] = VertexInterp(isoLevel, cubePos[0], cubePos[4], cubeVal[0], cubeVal[4]);
                    colorList[8]  = ColorInterp(isoLevel, cubeColor[0], cubeColor[4], cubeVal[0], cubeVal[4]);
                }
                if (edges & 512) {
                    vertexList[9] = VertexInterp(isoLevel, cubePos[1], cubePos[5], cubeVal[1], cubeVal[5]);
                    colorList[9]  = ColorInterp(isoLevel, cubeColor[1], cubeColor[5], cubeVal[1], cubeVal[5]);
                }
                if (edges & 1024) {
                    vertexList[10] = VertexInterp(isoLevel, cubePos[2], cubePos[6], cubeVal[2], cubeVal[6]);
                    colorList[10]  = ColorInterp(isoLevel, cubeColor[2], cubeColor[6], cubeVal[2], cubeVal[6]);
                }
                if (edges & 2048) {
                    vertexList[11] = VertexInterp(isoLevel, cubePos[3], cubePos[7], cubeVal[3], cubeVal[7]);
                    colorList[11]  = ColorInterp(isoLevel, cubeColor[3], cubeColor[7], cubeVal[3], cubeVal[7]);
                }
                for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
                    int i0 = vertices.size();
                    vertices.push_back({ vertexList[triTable[cubeIndex][i]],     glm::vec3(0), colorList[triTable[cubeIndex][i]] });
                    vertices.push_back({ vertexList[triTable[cubeIndex][i + 1]], glm::vec3(0), colorList[triTable[cubeIndex][i + 1]] });
                    vertices.push_back({ vertexList[triTable[cubeIndex][i + 2]], glm::vec3(0), colorList[triTable[cubeIndex][i + 2]] });
                    indices.push_back(i0);
                    indices.push_back(i0 + 1);
                    indices.push_back(i0 + 2);
                }
            }
        }
    }
}
void calcularNormales(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    for (auto& v : vertices)
        v.normal = glm::vec3(0.0f);
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        const glm::vec3& p0 = vertices[i0].position;
        const glm::vec3& p1 = vertices[i1].position;
        const glm::vec3& p2 = vertices[i2].position;
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 triangleNormal = glm::normalize(glm::cross(edge1, edge2));
        vertices[i0].normal += triangleNormal;
        vertices[i1].normal += triangleNormal;
        vertices[i2].normal += triangleNormal;
    }
    for (auto& v : vertices)
        if (glm::length(v.normal) > 0.0f)
            v.normal = glm::normalize(v.normal);
}

// ================== MENÚ Y RECARGA EN TIEMPO REAL ===================
vector< vector<Punto3D> > puntos_por_mascara;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        for (size_t i = 0; i < mascara_activa.size(); ++i) {
            int code = (i < 9) ? GLFW_KEY_1 + i : GLFW_KEY_A + (i-9);
            if (key == code) {
                mascara_activa[i] = !mascara_activa[i];
                printMascaraStatus();
            }
        }
        if (key == GLFW_KEY_R) {
            glfwSetWindowShouldClose(window, 2); // señal especial para recargar malla
        }
    }
}

int main() {
    if (!glfwInit()) {
        cerr << "Error - INICIALIZAR GLFW" << endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    int ancho = 2100, alto = 1200;
    GLFWwindow* ventana = glfwCreateWindow(ancho, alto, "Ranita bonita", nullptr, nullptr);
    if(!ventana){
        cerr<<"Error - CREAR VENTANA";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(ventana);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetMouseButtonCallback(ventana, mouse_button_callback);
    glfwSetCursorPosCallback(ventana, cursor_position_callback);
    glfwSetScrollCallback(ventana, scroll_callback);
    glfwSetKeyCallback(ventana, key_callback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    GLuint shaderProgram = crearShaderProgram();

    // --------- CARGA DE MÁSCARAS EN MEMORIA (solo una vez) -----------
    puntos_por_mascara.resize(mascaras.size());
    string ruta_base = "ImgsFormateo/salida_pngs";
    string extension = "_frame_";
    string extension2 = ".png";

    for (size_t mi = 0; mi < mascaras.size(); ++mi) {
        string mascara = mascaras[mi];
        cout << "Procesando mascara: " << mascara << endl;
        for (int i = 1; i <= 136; ++i) {
            string ruta_img =  ruta_base + "/" + mascara + extension + to_string(i) + extension2;
            cv::Mat img = cv::imread(ruta_img, cv::IMREAD_GRAYSCALE);
            if (img.empty()) continue;
            // Si quieres filtro especial para alguna máscara, ponlo aquí
            for (int y = 0; y < img.rows; ++y) {
                for (int x = 0; x < img.cols; ++x) {
                    if (img.at<uchar>(y, x) > 127) {
                        puntos_por_mascara[mi].push_back({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(i), mascara_colors[mi] });
                    }
                }
            }
        }
    }

    printMascaraStatus();

    // =============== CICLO PRINCIPAL: solo combinamos las máscaras activas ============
    while (true) {
        // Combinar puntos de las máscaras activas
        vector<Punto3D> puntos_totales;
        for (size_t mi = 0; mi < mascaras.size(); ++mi)
            if (mascara_activa[mi])
                puntos_totales.insert(puntos_totales.end(), puntos_por_mascara[mi].begin(), puntos_por_mascara[mi].end());
        cout << "Puntos totales: " << puntos_totales.size() << endl;

        // --- PASO 1: Calcular el volumen 3D binario y color ---
        int maxX = 0, maxY = 0, maxZ = 0;
        for (const auto& p : puntos_totales) {
            maxX = std::max(maxX, (int)p.x);
            maxY = std::max(maxY, (int)p.y);
            maxZ = std::max(maxZ, (int)p.z);
        }
        int VOLUME_WIDTH = maxX + 1;
        int VOLUME_HEIGHT = maxY + 1;
        int VOLUME_DEPTH = maxZ + 1;

        vector<vector<vector<uint8_t>>> volumen(
            VOLUME_DEPTH, vector<vector<uint8_t>>(VOLUME_HEIGHT, vector<uint8_t>(VOLUME_WIDTH, 0))
        );
        vector<vector<vector<glm::vec3>>> volumen_color(
            VOLUME_DEPTH, vector<vector<glm::vec3>>(VOLUME_HEIGHT, vector<glm::vec3>(VOLUME_WIDTH, glm::vec3(0)))
        );
        for (const auto& p : puntos_totales) {
            int x = static_cast<int>(p.x);
            int y = static_cast<int>(p.y);
            int z = static_cast<int>(p.z);
            if (x >= 0 && x < VOLUME_WIDTH &&
                y >= 0 && y < VOLUME_HEIGHT &&
                z >= 0 && z < VOLUME_DEPTH)
            {
                volumen[z][y][x] = 1;
                volumen_color[z][y][x] = p.color;
            }
        }

        // --- PASO 2: Generar malla con Marching Cubes ---
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        MarchingCubes(volumen, volumen_color, vertices, indices);
        calcularNormales(vertices, indices);

        GLuint VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));

        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Render loop
        bool recargar = false;
        while (!glfwWindowShouldClose(ventana)) {
            glfwPollEvents();
            if (glfwWindowShouldClose(ventana) == 2) { // señal especial
                glfwSetWindowShouldClose(ventana, 0);
                recargar = true;
                break;
            }
            glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(shaderProgram);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(translateX, translateY, 0.0f));
            model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(zoom));
            model = glm::translate(model, glm::vec3(-128.0f, -128.0f, -68.0f));
            glm::vec3 modeloCentro(128.0f, 128.0f, 68.0f);
            glm::vec3 camaraPos = modeloCentro + glm::vec3(0, 0, 500);
            glm::vec3 camaraFrente = modeloCentro;
            glm::vec3 camaraArriba = glm::vec3(0, 1, 0);
            glm::mat4 view = glm::lookAt(camaraPos, camaraFrente, camaraArriba);
            glm::mat4 proj = glm::perspective(glm::radians(60.0f), ancho / float(alto), 0.1f, 2000.0f);
            GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
            GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
            GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

            // Luz y cámara para Phong
            GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
            GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
            glUniform3f(lightPosLoc, 128.0f, 128.0f, 200.0f);
            glUniform3f(viewPosLoc, camaraPos.x, camaraPos.y, camaraPos.z);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

            glfwSwapBuffers(ventana);
        }
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
        if (!recargar) break;
    }
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(ventana);
    glfwTerminate();
    return 0;
}