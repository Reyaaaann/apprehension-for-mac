// Prints the parameter table and presets as JSON (used by gen_docs.py so the manual always matches the plugin).
#include <cstdio>
#include "../src/presets.h"
static void esc(const char* s) { for (; *s; ++s) { if (*s == '"' || *s == '\\') std::putchar('\\'); std::putchar(*s); } }
int main() {
    std::printf("{\"params\":[");
    for (int i = 0; i < P_COUNT; ++i) {
        const ParamSpec& s = kSpec[i];
        std::printf("%s{\"id\":\"%s\",\"name\":\"", i ? "," : "", kIds[i]); esc(s.name);
        std::printf("\",\"kind\":%d,\"min\":%g,\"max\":%g,\"def\":%g,\"fmt\":%d,\"choices\":[", s.kind, s.mn, s.mx, s.def, s.fmt);
        for (int c = 0; c < s.nChoices; ++c) { std::printf("%s\"", c ? "," : ""); esc(s.choices[c]); std::printf("\""); }
        std::printf("]}");
    }
    std::printf("],\"presets\":[");
    for (size_t k = 0; k < presets().size(); ++k) {
        const Preset& p = presets()[k];
        std::printf("%s{\"name\":\"", k ? "," : ""); esc(p.name); std::printf("\",\"desc\":\""); esc(p.desc); std::printf("\",\"values\":{");
        for (size_t v = 0; v < p.values.size(); ++v) std::printf("%s\"%s\":%g", v ? "," : "", kIds[p.values[v].id], p.values[v].v);
        std::printf("}}");
    }
    std::printf("]}\n");
}
