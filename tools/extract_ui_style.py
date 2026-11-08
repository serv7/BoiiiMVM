from pathlib import Path
p=Path('src/plugin.cpp');s=p.read_text();start=s.index('    ImGui::StyleColorsDark();');end=s.index('    if(!ImGui_ImplWin32_Init',start);style=s[start:end];s=s[:start]+'    theater::configure_ui();\n'+s[end:];p.write_text(s)
p=Path('src/ui.cpp');s=p.read_text();s=s.replace('void draw_ui() {','void configure_ui() {\n'+style+'}\nvoid draw_ui() {');s=s.replace('#include <string>','#include <string>\n#include <filesystem>');p.write_text(s)
p=Path('src/backend.hpp');s=p.read_text().replace('void draw_ui();','void configure_ui();\nvoid draw_ui();');p.write_text(s)
