# compile for debugging. optimize for fastest compile speed.
mkdir -p build
emcc --std=c++11 -s WASM=1 -lGL imgui.cpp imgui_draw.cpp imgui_widgets.cpp imgui_demo.cpp imgui-emsc.cpp jello.cpp renderer.cpp world.cpp ground.cpp main.cpp -o build/muhu.html --shell-file html_template/shell_minimal.html
