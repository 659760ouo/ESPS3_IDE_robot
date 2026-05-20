import os
import re
from PIL import Image

def clean_variable_name(filename):
    """Converts file names into valid, clean C++ variable identifiers.
    
    Modified to seamlessly align with HKO API condition string outputs.
    """
    name = os.path.splitext(filename)[0] # Extract the filename core
    
    # Standardize spaces, dashes, and brackets into clean underscores
    name = name.lower()
    name = name.replace("(", "_").replace(")", "_")
    name = re.sub(r'[^a-z0-9_]', '_', name)
    
    # Handle numeric edge cases
    if name.isdigit():
        name = "img_" + name
        
    # Ensure standard suffix array binding
    if not name.endswith("_png"):
        name += "_png"
        
    # Reduce consecutive underscores caused by punctuation replacement (e.g., "fine(night)" -> "fine_night__png")
    name = re.sub(r'_+', '_', name)
    return name

def generate_img_header(input_folder, output_file_path, target_size=(48, 48)):
    if not os.path.exists(input_folder):
        print(f"❌ Error: Input folder '{input_folder}' does not exist.")
        return

    png_files = [f for f in os.listdir(input_folder) if f.lower().endswith('.png')]
    
    if not png_files:
        print(f"⚠️ Warning: No PNG files found inside '{input_folder}'.")
        return

    print(f"📦 Found {len(png_files)} PNG assets. Commencing auto-resize and compilation...")

    # Open the target file once outside the loop
    with open(output_file_path, "w", encoding="utf-8") as f:
        # Write standard header guards
        f.write("#ifndef IMG_H\n")
        f.write("#define IMG_H\n\n")
        f.write("#include <pgmspace.h> // Required for PROGMEM on ESP32 frameworks\n\n")
        
        for file_name in png_files:
            file_path = os.path.join(input_folder, file_name)
            var_name = clean_variable_name(file_name)
            
            # --- AUTO RESIZE BLOCK ---
            # Open the image using PIL, resize it to 48x48 pixels, and save it as a temporary small PNG
            temp_output = "temp_small.png"
            with Image.open(file_path) as img:
                # Resize using high-quality resampling filters
                img_resized = img.resize(target_size, Image.Resampling.LANCZOS)
                img_resized.save(temp_output, "PNG")
            
            # Read the newly compressed, small binary data stream
            with open(temp_output, "rb") as png_file:
                binary_data = png_file.read()
            
            # Clean up the temporary asset file
            if os.path.exists(temp_output):
                os.remove(temp_output)
                
            print(f"  -> Successfully Resized & Compiled: '{file_name}' ({target_size[0]}x{target_size[1]}) to '{var_name}' ({len(binary_data)} bytes)")

            f.write(f"// Raw compressed PNG asset chunk sourced from: {file_name}\n")
            f.write(f"const uint8_t {var_name}[] PROGMEM = {{\n  ")
            
            # Format bytes as hex array blocks formatted 12 bytes per text row
            for index, byte in enumerate(binary_data):
                f.write(f"0x{byte:02x}, ")
                if (index + 1) % 12 == 0 and (index + 1) != len(binary_data):
                    f.write("\n  ")
            
            f.write("\n};\n\n")
        
        f.write("#endif // IMG_H\n")
        
    print(f"✅ Success! Your compiled file has been generated at: {output_file_path}")

if __name__ == "__main__":
    INPUT_DIR = "./icons" 
    OUTPUT_FILE = "./img.h"
    
    # Target size matching your circular weather display area layout (48x48 pixels)
    TARGET_PIXEL_SIZE = (48, 48) 
    
    if not os.path.exists(INPUT_DIR):
        os.makedirs(INPUT_DIR)
        print(f"📁 Created '{INPUT_DIR}' directory. Put your source PNG files inside it and run again!")
    else:
        generate_img_header(INPUT_DIR, OUTPUT_FILE, TARGET_PIXEL_SIZE)
