#include "render/Mesh.h"

namespace render {
Mesh Mesh::makeTriangle() {
    Mesh m;
    m.vertices_ = {
        //   pos        color
        {{ 0.0f, -0.5f }, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f }, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f }, {0.0f, 0.0f, 1.0f}},
    };
    m.indices_ = {0,1,2};
    return m;
}
}
