#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <utility> // Para std::pair
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>

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

// ---- Variables globales ----
string mascaras[] = {
    //"bloodMasks",
    //"BrainMasks",
    //"duodenumMasks",
    //"eyeMasks",
    //"eyeRetnaMasks",
    //"eyeWhiteMasks",
    //"heartMasks",
    //"ileumMasks",
    //"kidneyMasks",
    //"lIntestineMasks",
    //"liverMasks",
    //"lungMasks",
    //"muscleMasks",
    //"nerveMasks",
    "skeletonMasks",
    //"spleenMasks",
    //"stomachMasks"
};

struct Punto3D {
    float x, y, z;
    float r, g, b;
};


// ---- Shaders ----
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 1;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main(){
	//FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Color rojo
	FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Color verde
	//FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Color azul
	//FragColor = vec4(1.0, 1.0, 1.0, 1.0); // Color blanco
	//FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Color amarillo
	//FragColor = vec4(1.0, 165/255.0f, 0/255.0f, 1.0); // Color naranja
	//FragColor = vec4(128/255.0f, 128/255.0f, 128/255.0f, 1.0); // Color gris
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

	string ruta_base = "C:\\Users\\rushe\\Documents\\Universidad\\S7\\Graphics\\CS-GRAFICA\\Laboratorio7\\salida_pngs";
	string extension = "_frame_";
	string extension2 = ".png";

	map<int, vector<Punto3D>> puntos_map_totales;
	vector<Punto3D> puntos_totales;

	int mascara_index = 1;
	for (const auto& mascara : mascaras) {
		cout << "Procesando mascara: " << mascara << endl;

		for (int i = 1; i <= 136; ++i) {
			string ruta_img =  ruta_base + "\\" + mascara + extension + to_string(i) + extension2;

			//cout << "Leyendo imagen: " << ruta_img << endl;
            cv::Mat img = cv::imread(ruta_img, cv::IMREAD_GRAYSCALE);
            for (int y = 0; y < img.rows; ++y) {
                for (int x = 0; x < img.cols; ++x) {
                    if (img.at<uchar>(y, x) > 127) {
						//cout << "Punto encontrado en: " << x << ", " << y << endl;
				
						puntos_totales.push_back({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(i) });
                    }
                }
            }
		}
        mascara_index++;
	}

	cout << "Puntos totales: " << puntos_totales.size() << endl;

    GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, puntos_totales.size() * sizeof(Punto3D), puntos_totales.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Punto3D), (void*)0);
    glBindVertexArray(0);

    // Shaders
    GLuint shaderProgram = crearShaderProgram();

    // Matrices
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)); // escala a la mitad

    // Interaccion
    glfwSetMouseButtonCallback(ventana, mouse_button_callback);
    glfwSetCursorPosCallback(ventana, cursor_position_callback);
    glfwSetScrollCallback(ventana, scroll_callback);


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
        glDrawArrays(GL_POINTS, 0, puntos_totales.size());

        glfwSwapBuffers(ventana);
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(ventana);
    glfwTerminate();
    return 0;
}
