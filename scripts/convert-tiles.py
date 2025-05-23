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