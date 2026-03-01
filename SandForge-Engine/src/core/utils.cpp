#include "utils.h"
#include "app/log.h"
#include <fstream>
#include <sstream>
#include <vector>

std::string ReadTextFile(const char* path) {
    std::ifstream f(path, std::ios::in);
    if (!f) {
        LOG("ERROR: ReadTextFile failed '%s'", path ? path : "<null>");
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}


uint MakeShader(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    LogShader(s);
    return s;
}
uint MakeProgram(const char* vs, const char* fs) {
    if (!vs || !vs[0] || !fs || !fs[0]) {
        LOG("ERROR: MakeProgram called with empty shader source");
    }
    unsigned int v = MakeShader(GL_VERTEX_SHADER, vs);
    unsigned int f = MakeShader(GL_FRAGMENT_SHADER, fs);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    LogProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static void LogShader(GLuint s) {
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint n = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &n);
        std::vector<char> log(n > 0 ? n : 1);
        GLsizei out = 0; glGetShaderInfoLog(s, n, &out, log.data());
        LOG("[GL] Shader compile error: %.*s", out, log.data());
    }
}

static void LogProgram(GLuint p) {
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint n = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &n);
        std::vector<char> log(n > 0 ? n : 1);
        GLsizei out = 0; glGetProgramInfoLog(p, n, &out, log.data());
        LOG("[GL] Program link error: %.*s", out, log.data());
    }
}