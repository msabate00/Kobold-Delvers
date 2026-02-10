
#include "sprite.h"
#include "texture.h"
#include "core/utils.h"     
#include <glad/gl.h>
#include <algorithm>

static inline void rgba32_to_bytes(uint32 c, unsigned char* outRGBA) {
    outRGBA[0] = (c >> 0) & 0xFF;
    outRGBA[1] = (c >> 8) & 0xFF;
    outRGBA[2] = (c >> 16) & 0xFF;
    outRGBA[3] = (c >> 24) & 0xFF;
}

void SpriteBatch::Init() {

    std::string vsSprite = ReadTextFile(SHADER_DIR "/sprite.vs.glsl");
    std::string fsSprite = ReadTextFile(SHADER_DIR "/sprite.fs.glsl");

    prog = MakeProgram(vsSprite.c_str(), fsSprite.c_str());
    glUseProgram(prog);
    loc_uView = glGetUniformLocation(prog, "uView");
    GLint loc_uTex = glGetUniformLocation(prog, "uTex");
    if (loc_uTex >= 0) glUniform1i(loc_uTex, 0);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);


    GLsizei stride = sizeof(V);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(V, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(V, u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(V, rgba));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    verts.reserve(4096);       
    queue.reserve(1024);
}

void SpriteBatch::Shutdown() {
    queue.clear(); verts.clear();
    if (vbo) glDeleteBuffers(1, &vbo), vbo = 0;
    if (vao) glDeleteVertexArrays(1, &vao), vao = 0;
    if (prog) glDeleteProgram(prog), prog = 0;
}

void SpriteBatch::Begin(int viewW_, int viewH_) {
    viewW = viewW_; viewH = viewH_;
    glUseProgram(prog);
    if (loc_uView >= 0) glUniform2f(loc_uView, (float)viewW, (float)viewH);
    glEnable(GL_BLEND);

    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void SpriteBatch::Push(const Sprite& s) {
    queue.push_back(s);
}

void SpriteBatch::Flush(RenderLayer layer) {
    if (queue.empty()) return;


    tmp.clear();
    tmp.reserve(queue.size());
    for (const auto& s : queue) if (s.layer == layer) tmp.push_back(&s);
    if (tmp.empty()) return;


    std::stable_sort(tmp.begin(), tmp.end(),
        [](const Sprite* a, const Sprite* b) { return a->tex < b->tex; });

    glUseProgram(prog);
    if (loc_uView >= 0) glUniform2f(loc_uView, (float)viewW, (float)viewH);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    size_t i = 0;
    while (i < tmp.size()) {
        const Texture2D* tex = tmp[i]->tex;
        glBindTexture(GL_TEXTURE_2D, tex ? tex->id : 0);


        size_t j = i + 1;
        while (j < tmp.size() && tmp[j]->tex == tex) ++j;
        const size_t count = j - i;


        verts.clear();
        verts.resize(count * 6);

        size_t v = 0;
        for (size_t k = i; k < j; ++k) {
            const Sprite& s = *tmp[k];

            float x0 = s.x, y0 = s.y;
            float x1 = s.x + s.w, y1 = s.y + s.h;
            float u0 = s.u0, v0 = s.v0, u1 = s.u1, v1 = s.v1;

            unsigned char rgba[4]; rgba32_to_bytes(s.color, rgba);
            V q[6] = {
                { x0,y0, u0,v0, *(uint32*)rgba },
                { x1,y0, u1,v0, *(uint32*)rgba },
                { x1,y1, u1,v1, *(uint32*)rgba },
                { x0,y0, u0,v0, *(uint32*)rgba },
                { x1,y1, u1,v1, *(uint32*)rgba },
                { x0,y1, u0,v1, *(uint32*)rgba },
            };
            
            for (int t = 0;t < 6;++t) verts[v++] = q[t];
        }

        
        const GLsizeiptr bytes = (GLsizeiptr)(verts.size() * sizeof(V));
        glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, GL_DYNAMIC_DRAW); 
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, verts.data());
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)verts.size());

        i = j;
    }

    glBindVertexArray(0);

    if (tmp.size() == queue.size()) {
        queue.clear();
    }
    else {
        std::vector<Sprite> rest;
        rest.reserve(queue.size() - tmp.size());
        for (const auto& s : queue) if (s.layer != layer) rest.push_back(s);
        queue.swap(rest);
    }
}
