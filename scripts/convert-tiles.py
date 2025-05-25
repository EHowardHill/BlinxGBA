import os, json

final = """// Level structure
struct level_ptr
{
    const regular_bg_item *bg_item;
    const int collisions[512];
    int size_x;
    int size_y;
    int init_x;
    int init_y;
};\n\n"""

for tile in os.listdir(os.path.join("tilesets", "maps_json")):
    with open(os.path.join("tilesets", "maps_json", tile), "r") as f:
        data = json.load(f)

    name = tile.replace(".tmj", "")
    grid = str(data["layers"][0]["data"]).replace("[", "{").replace("]", "}")
    width = data["layers"][0]["width"]
    height = data["layers"][0]["height"]
    template = f"const level_ptr {name} = {{&regular_bg_items::{name}, {grid}, {width}, {height}, 0, 0}};"

    final += f"#include <bn_regular_bg_items_{name}.h>\n"
    final += template + "\n\n"

with open(os.path.join("src", "maps.h"), "w") as f:
    f.write(final)

from PIL import Image
import numpy as np

for image in os.listdir("graphics/maps_raw/"):
    file_path = "graphics/maps_raw/" + image
    
    # Open the original image
    try:
        img = Image.open(file_path)
    except Exception as e:
        raise ValueError(f"Could not open image file: {e}")
    
    # Convert to RGB if necessary
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Get original dimensions
    width, height = img.size
    
    # Find the larger dimension and round up to nearest multiple of 256
    max_dim = max(width, height)
    target_size = ((max_dim + 255) // 256) * 256
    
    # Create new square image with black background
    square_img = Image.new('RGB', (target_size, target_size), (0, 0, 0))
    
    # Calculate position to center the original image
    x_offset = (target_size - width) // 2
    y_offset = (target_size - height) // 2
    
    # Paste the original image onto the black square
    square_img.paste(img, (x_offset, y_offset))
    
    # Convert to indexed color with maximum 16 colors
    # First, we need to quantize the image
    quantized_img = square_img.quantize(colors=16, method=Image.Quantize.MEDIANCUT)
    
    # Get the palette and ensure black is first
    palette = quantized_img.getpalette()
    
    # Convert palette to list of RGB tuples
    rgb_palette = []
    for i in range(0, len(palette), 3):
        rgb_palette.append((palette[i], palette[i+1], palette[i+2]))
    
    # Check if black is already in palette, if not add it
    black = (0, 0, 0)
    if black not in rgb_palette:
        # Replace the last color with black if we're at 16 colors
        if len(rgb_palette) >= 16:
            rgb_palette = rgb_palette[:15]
        rgb_palette.append(black)
    
    # Move black to first position if it's not already there
    if rgb_palette[0] != black:
        rgb_palette.remove(black)
        rgb_palette.insert(0, black)
    
    # Create new palette with black first
    new_palette = []
    for r, g, b in rgb_palette:
        new_palette.extend([r, g, b])
    
    # Pad palette to 256 colors (required for some BMP readers)
    while len(new_palette) < 768:  # 256 * 3
        new_palette.extend([0, 0, 0])
    
    # Create a new image with the reordered palette
    final_img = Image.new('P', (target_size, target_size))
    final_img.putpalette(new_palette)
    
    # Map pixels to new palette indices
    square_array = np.array(square_img)
    indexed_array = np.zeros((target_size, target_size), dtype=np.uint8)
    
    for y in range(target_size):
        for x in range(target_size):
            pixel = tuple(square_array[y, x])
            # Find closest color in palette
            min_dist = float('inf')
            best_index = 0
            for i, pal_color in enumerate(rgb_palette):
                dist = sum((a - b) ** 2 for a, b in zip(pixel, pal_color))
                if dist < min_dist:
                    min_dist = dist
                    best_index = i
            indexed_array[y, x] = best_index
    
    # Create final indexed image
    final_img = Image.fromarray(indexed_array, mode='P')
    final_img.putpalette(new_palette)
    
    # Prepare output path
    base_name = os.path.splitext(os.path.basename(file_path))[0]
    output_dir = "graphics/maps"
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, f"{base_name}.bmp")
    
    # Save as BMP without color space information
    # Use optimize=False to avoid color space info
    final_img.save(output_path, format='BMP', optimize=False)

    with open(f"graphics/maps/{base_name}.json", "w") as f:
        f.write("""{
            "type": "regular_bg"
        }""")
    
    print(f"Image converted and saved to: {output_path}")
    print(f"Final size: {target_size}x{target_size}")
    print(f"Colors in palette: {len(rgb_palette)}")

# Example usage:
# convert_to_square_bitmap("input_image.jpg")