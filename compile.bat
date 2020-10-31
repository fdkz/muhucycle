@rem fastest compile 909k
@rem emcc --std=c++11 -s WASM=1 -lGL imgui.cpp imgui_draw.cpp imgui-emsc.cpp -o build/muhu.html --shell-file html_template/shell_minimal.html
emcc --std=c++11 -s WASM=1 -lGL imgui.cpp imgui_draw.cpp imgui-emsc.cpp jello.cpp main.cpp -o build/muhu.html --shell-file html_template/shell_minimal.html

@rem small 375k
@rem emcc --std=c++11 -O2 -s WASM=1 --closure 1 -lGL imgui.cpp imgui_draw.cpp imgui-emsc.cpp -o build/muhu.html --shell-file html_template/shell_minimal.html

@rem smallest 285k
@rem emcc --std=c++11 -Os -s WASM=1 --closure 1 -lGL imgui.cpp imgui_draw.cpp imgui-emsc.cpp -o build/muhu.html --shell-file html_template/shell_minimal.html
