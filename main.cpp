#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    if (!glfwInit()) {
        std::cerr << "Error al iniciar GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Reconstrucción 3D", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error al crear ventana GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error al cargar funciones OpenGL con GLAD\n";
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    // skeletonMasks.tiff
    // Probar OpenCV"C:\Users\rushe\Documents\Universidad\S7\Graphics\CS-GRAFICA\Lab7\x64\Debug\resources\bloodMasks.tiff"
    std::string ruta = "C:/Users/rushe/Documents/Universidad/S7/Graphics/CS-GRAFICA/Laboratorio7/resources/bloodMasks.tiff";
    std::cout << "Cargando desde: " << ruta << std::endl;

    cv::Mat img = cv::imread(ruta, cv::IMREAD_GRAYSCALE);

    if (img.empty())
        std::cerr << "Error cargando imagen TIFF desde: " << ruta << std::endl;
    else
        std::cout << "Imagen cargada: " << img.cols << "x" << img.rows << std::endl;


    //cv::Mat img = cv::imread("C:/Users/rushe/Documents/Universidad/S7/Graphics/CS-GRAFICA/Lab7/x64/Debug/resources/skeletonMasks.tiff", cv::IMREAD_GRAYSCALE);
//    if (!img.empty())
//        std::cout << "Imagen TIFF cargada: " << img.cols << "x" << img.rows << "\n";
 //   else
   //     std::cerr << "Error cargando imagen TIFF\n";

    return 0;
}
