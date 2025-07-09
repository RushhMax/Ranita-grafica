from PIL import Image
import os

# Carpeta con los archivos TIFF de entrada
input_folder = 'resources'
# Carpeta donde se guardarán las imágenes PNG resultantes
output_folder = 'salida_pngs'

# Crear carpeta de salida si no existe
os.makedirs(output_folder, exist_ok=True)

# Procesar todos los archivos .tiff o .tif en la carpeta
for filename in os.listdir(input_folder):
    if filename.lower().endswith(('.tif', '.tiff')):
        tiff_path = os.path.join(input_folder, filename)
        base_name = os.path.splitext(filename)[0]

        try:
            with Image.open(tiff_path) as img:
                for i in range(img.n_frames):
                    img.seek(i)
                    salida_nombre = f'{base_name}_frame_{i+1}.png'
                    output_path = os.path.join(output_folder, salida_nombre)
                    img.save(output_path)
                    print(f'Guardado: {output_path}')
        except Exception as e:
            print(f'Error al procesar {tiff_path}: {e}')
