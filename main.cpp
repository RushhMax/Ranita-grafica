#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>


using namespace std;


string mascaras[] = { "brainMasks"

};

struct Punto3D {
	float x, y, z;
};

int main() {
    // C:\Users\rushe\Documents\Universidad\S7\Graphics\CS-GRAFICA\Laboratorio7\salida_pngs
	string ruta_base = "C:/Users/rushe/Documents/Universidad/S7/Graphics/CS-GRAFICA/Laboratorio7/salida_pngs";

	// Cargar las mascaras
	string extension = "_frame_";
	string extension2 = ".png";
	for (const auto& mascara : mascaras) {
		cout << "Procesando mascara: " << mascara << endl;
		for (int i = 1; i <= 1; ++i) {
            std::vector<Punto3D> puntos;
			string ruta_img = ruta_base + "/" + mascara + extension + to_string(i) + extension2;

			cout << "Leyendo imagen: " << ruta_img << endl;
            cv::Mat img = cv::imread(ruta_img, cv::IMREAD_GRAYSCALE);
            for (int y = 0; y < img.rows; ++y) {
                for (int x = 0; x < img.cols; ++x) {
                    if (img.at<uchar>(y, x) > 127) {
						cout << "Punto encontrado en: " << x << ", " << y << endl;
                        puntos.push_back({ (float)x, (float)(img.rows - y), (float)i });
                    }
                }
            }


		}
	}

    return 0;
}
