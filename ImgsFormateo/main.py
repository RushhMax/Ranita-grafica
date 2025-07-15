from PIL import Image
import os
import zipfile

script_dir = os.path.dirname(os.path.abspath(__file__))
zip_path = os.path.join(script_dir, 'imagenT.zip')
temp_extract_folder = os.path.join(script_dir, 'temp_imgs')  # Carpeta temporal
output_folder = os.path.join(script_dir, 'salida_pngs')

# Crear carpetas si no existen
os.makedirs(temp_extract_folder, exist_ok=True)
os.makedirs(output_folder, exist_ok=True)

# Descomprimir ZIP
with zipfile.ZipFile(zip_path, 'r') as zip_ref:
    zip_ref.extractall(temp_extract_folder)

# Procesar los archivos TIFF extraídos
for filename in os.listdir(temp_extract_folder):
    if filename.lower().endswith(('.tif', '.tiff')):
        tiff_path = os.path.join(temp_extract_folder, filename)
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
