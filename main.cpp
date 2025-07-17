#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <utility> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>

#include "marching_cubes_tables.h"


using namespace std;


// VARIABLES DE MOVIMIENTO 
bool mousePressed = false;
double lastX = 0.0, lastY = 0.0;
float yaw = 0.0f, pitch = 0.0f;
float translateX = 0.0f, translateY = 0.0f;
float angulo_rotar = 0.0f;
float zoom = 1.0f;

// INTERACCION
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

glm::vec3 mascara_colors[] = {
    {1, 0, 0},       // bloodMasks: Máscaras de sangre - rojo
    {0, 1, 0},       // BrainMasks: Máscaras de cerebro - verde
    {0, 0, 1},       // duodenumMasks: Máscaras de duodeno - azul
    {1, 1, 0},       // eyeMasks: Máscaras de ojo - amarillo
    {1, 0, 1},       // eyeRetnaMasks: Máscaras de retina del ojo - magenta
    {1, 1, 1},       // eyeWhiteMasks: Máscaras de esclerótica (parte blanca del ojo) - blanco
    {1, 0.5, 0},     // heartMasks: Máscaras de corazón - naranja
    {0.5, 0, 1},     // ileumMasks: Máscaras de íleon - violeta
    {0.5, 1, 0},     // kidneyMasks: Máscaras de riñón - verde lima
    {0, 0.5, 1},     // lIntestineMasks: Máscaras de intestino grueso - azul celeste
    {1, 0.2, 0.8},   // liverMasks: Máscaras de hígado - rosado fuerte
    {0.2, 1, 0.8},   // lungMasks: Máscaras de pulmón - verde aguamarina
    {0.8, 0.8, 0.2}, // muscleMasks: Máscaras de músculo - amarillo mostaza
    {0.8, 0.2, 0.8}, // nerveMasks: Máscaras de nervio - púrpura claro
    {0.2, 0.8, 0.8}, // skeletonMasks: Máscaras de esqueleto - celeste verdoso
    {0.3, 0.7, 0.2}, // spleenMasks: Máscaras de bazo - verde musgo
    {0.9, 0.4, 0.1}, // stomachMasks: Máscaras de estómago - naranja quemado

};


// ---- Variables globales ----
string mascaras[] = {
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

struct Punto3D {
    float x, y, z;
    glm::vec3 color;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal = glm::vec3(0.0f);  // inicializamos normal en cero
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f); // Agrega atributo color
};

// ---- Shaders ----
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
out vec3 ourColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 1;
    ourColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;

void main(){
	//FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Color rojo
	//FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Color verde
	//FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Color azul
	//FragColor = vec4(1.0, 1.0, 1.0, 1.0); // Color blanco
	//FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Color amarillo
	//FragColor = vec4(1.0, 165/255.0f, 0/255.0f, 1.0); // Color naranja
	//FragColor = vec4(128/255.0f, 128/255.0f, 128/255.0f, 1.0); // Color gris
    FragColor = vec4(ourColor, 1.0);
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

// Interpolador de vértices
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


// Este esqueleto se puede refinar más adelante
void MarchingCubes(const vector<vector<vector<uint8_t>>>& volumen,
                   const vector<vector<vector<glm::vec3>>>& volumen_color,
                   vector<Vertex>& vertices,
                   vector<unsigned int>& indices,
                   float isoLevel = 0.5f)
{
    // Determina el tamaño del volumen 3D (ancho, alto, profundidad
    int width = volumen[0][0].size();
    int height = volumen[0].size();
    int depth = volumen.size();

    glm::vec3 vertexList[12];
    glm::vec3 colorList[12];

    // Recorrer el volumen por cada cubo 2x2x2
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

                // Determinar el índice del cubo en la tabla
                int cubeIndex = 0;
                for (int i = 0; i < 8; ++i)
                    if (cubeVal[i] > isoLevel) cubeIndex |= (1 << i);

                int edges = edgeTable[cubeIndex];
                if (edges == 0) continue;

                // Interpolar los vértices sobre las aristas
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

                // Crear triángulos
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

    cout << "[INFO] Total de vértices generados: " << vertices.size() << endl;
    cout << "[INFO] Total de triángulos generados: " << indices.size() / 3 << endl;


}


void calcularNormales(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    // Limpiar normales
    for (auto& v : vertices)
        v.normal = glm::vec3(0.0f);

    // Recorrer cada triángulo
    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        v0.normal += normal;
        v1.normal += normal;
        v2.normal += normal;
    }

    // Normalizar
    for (auto& v : vertices)
        v.normal = glm::normalize(v.normal);
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

	glfwMakeContextCurrent(ventana); // Hacer el contexto actual
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); // Cargar funciones de OpenGL
    
    // Interaccion
    glfwSetMouseButtonCallback(ventana, mouse_button_callback);
    glfwSetCursorPosCallback(ventana, cursor_position_callback);
    glfwSetScrollCallback(ventana, scroll_callback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

	string ruta_base = "ImgsFormateo/salida_pngs";
	string extension = "_frame_";
	string extension2 = ".png";

	map<int, vector<Punto3D>> puntos_map_totales;
	vector<Punto3D> puntos_totales;

	int mascara_index = 1;
	for (const auto& mascara : mascaras) {
		cout << "Procesando mascara: " << mascara << endl;

		for (int i = 1; i <= 136; ++i) {
			string ruta_img =  ruta_base + "/" + mascara + extension + to_string(i) + extension2;

			//cout << "Leyendo imagen: " << ruta_img << endl;
            cv::Mat img = cv::imread(ruta_img, cv::IMREAD_GRAYSCALE);
            for (int y = 0; y < img.rows; ++y) {
                for (int x = 0; x < img.cols; ++x) {
                    if (img.at<uchar>(y, x) > 127) {
						//cout << "Punto encontrado en: " << x << ", " << y << endl;
				
						//puntos_totales.push_back({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(i) });
                        puntos_totales.push_back({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(i), mascara_colors[mascara_index - 1] });
                    }
                }
            }
		}
        mascara_index++;
	}

	cout << "Puntos totales: " << puntos_totales.size() << endl;

    // --- PASO 1: Crear volumen 3D binario ---
    int maxX = 0, maxY = 0, maxZ = 0;
    for (const auto& p : puntos_totales) {
        maxX = std::max(maxX, (int)p.x);
        maxY = std::max(maxY, (int)p.y);
        maxZ = std::max(maxZ, (int)p.z);
    }
    int VOLUME_WIDTH = maxX + 1;
    int VOLUME_HEIGHT = maxY + 1;
    int VOLUME_DEPTH = maxZ*2 + 1;


    vector<vector<vector<uint8_t>>> volumen(
        VOLUME_DEPTH,
        vector<vector<uint8_t>>(VOLUME_HEIGHT, vector<uint8_t>(VOLUME_WIDTH, 0))
    );

    vector<vector<vector<glm::vec3>>> volumen_color(
        VOLUME_DEPTH, vector<vector<glm::vec3>>(VOLUME_HEIGHT, vector<glm::vec3>(VOLUME_WIDTH, glm::vec3(0)))
    );

    // Rellenar el volumen
    for (const auto& p : puntos_totales) {
        int x = static_cast<int>(p.x);
        int y = static_cast<int>(p.y);
        int z = static_cast<int>(p.z);
        if (x >= 0 && x < VOLUME_WIDTH &&
            y >= 0 && y < VOLUME_HEIGHT &&
            z >= 0 && z < VOLUME_DEPTH)
        {
            volumen[z][y][x] = 1;
            volumen[z + 1][y][x] = 1;  // duplicación en z

            volumen_color[z][y][x] = p.color;
            volumen_color[z + 1][y][x] = p.color;

        } else {
            cout << "[WARNING] Punto fuera del volumen: (" << x << "," << y << "," << z << ")" << endl;
        }

    }

    // --- PASO 2: Generar malla con Marching Cubes ---
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    MarchingCubes(volumen, volumen_color, vertices, indices);
    calcularNormales(vertices, indices);
    //cout << "[INFO_main] Total de vértices generados: " << vertices.size() << endl;
    //cout << "[INFO_main] Total de triángulos generados: " << indices.size() / 3 << endl;

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Vértices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Índices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Layout de atributos
    // posición
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // normales
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));

        // color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));



    // Shaders
    GLuint shaderProgram = crearShaderProgram();

    // Interaccion
    glfwSetMouseButtonCallback(ventana, mouse_button_callback);
    glfwSetCursorPosCallback(ventana, cursor_position_callback);
    glfwSetScrollCallback(ventana, scroll_callback);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // para dibujar superficies sólidas
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  // esto dibuja solo bordes de triángulos (rejilla)

    // Render loop
    while (!glfwWindowShouldClose(ventana)) {
        glfwPollEvents();
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);  // ¡Antes de enviar las matrices!

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(translateX, translateY, 0.0f)); // Movimiento con mouse
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));   // Yaw
        model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
        model = glm::scale(model, glm::vec3(zoom)); // Zoom
        model = glm::translate(model, glm::vec3(-128.0f, -128.0f, -68.0f)); // Centrado

        glm::vec3 modeloCentro(128.0f, 128.0f, 68.0f);

        glm::vec3 camaraPos = modeloCentro + glm::vec3(0, 0, 500); // cámara detrás del modelo
        glm::vec3 camaraFrente = modeloCentro;                       // mira al centro
        glm::vec3 camaraArriba = glm::vec3(0, 1, 0);                 // eje Y hacia arriba

        glm::mat4 view = glm::lookAt(camaraPos, camaraFrente, camaraArriba);


        glm::mat4 proj = glm::perspective(glm::radians(60.0f), ancho / float(alto), 0.1f, 2000.0f);

        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(ventana);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(ventana);
    glfwTerminate();
    return 0;
}