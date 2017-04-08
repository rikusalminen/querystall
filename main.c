#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdint.h>

#define STR(x) case x: return #x;
static const char *gl_debug_source_string(GLenum source) {
    switch(source) {
        STR(GL_DEBUG_SOURCE_API)
        STR(GL_DEBUG_SOURCE_WINDOW_SYSTEM)
        STR(GL_DEBUG_SOURCE_SHADER_COMPILER)
        STR(GL_DEBUG_SOURCE_THIRD_PARTY)
        STR(GL_DEBUG_SOURCE_APPLICATION)
        STR(GL_DEBUG_SOURCE_OTHER)
        default: break;
    }

    return "";
}

static const char *gl_debug_type_string(GLenum type) {
    switch(type) {
        STR(GL_DEBUG_TYPE_ERROR)
        STR(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
        STR(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
        STR(GL_DEBUG_TYPE_PORTABILITY)
        STR(GL_DEBUG_TYPE_PERFORMANCE)
        STR(GL_DEBUG_TYPE_OTHER)
        STR(GL_DEBUG_TYPE_MARKER)
        STR(GL_DEBUG_TYPE_PUSH_GROUP)
        STR(GL_DEBUG_TYPE_POP_GROUP)
        default: break;
    }

    return "";
}

static const char *gl_debug_severity_string(GLenum severity) {
    switch(severity) {
        STR(GL_DEBUG_SEVERITY_HIGH)
        STR(GL_DEBUG_SEVERITY_MEDIUM)
        STR(GL_DEBUG_SEVERITY_LOW)
        STR(GL_DEBUG_SEVERITY_NOTIFICATION)
        default: break;
    }

    return "";
}
#undef STR

static void APIENTRY gl_debug_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *param) {
    (void)length;
    (void)param;

    fprintf(stderr,
        "%s %s %s  id: 0x%x\n"
        "%s\n",
        gl_debug_source_string(source),
        gl_debug_type_string(type),
        gl_debug_severity_string(severity),
        id,
        message);
}


#define ROUNDROBIN_FRAMES 8
#define QUERY_COUNT 3

static unsigned int query_objects[ROUNDROBIN_FRAMES * QUERY_COUNT];

static const GLenum query_targets[QUERY_COUNT] = {
    GL_TIME_ELAPSED,
    GL_SAMPLES_PASSED,
    GL_PRIMITIVES_GENERATED
};

static const char* query_target_strings[QUERY_COUNT] = {
    "GL_TIME_ELAPSED",
    "GL_SAMPLES_PASSED",
    "GL_PRIMITIVES_GENERATED"
};

static int write_idx = 0, read_idx = 0;

static void init() {
    glGenQueries(ROUNDROBIN_FRAMES * QUERY_COUNT, query_objects);
}

static void begin_query() {

    printf("begin query round robin: %d\n", write_idx);

    for(int i = 0; i < QUERY_COUNT; ++i) {
        unsigned int query_handle = query_objects[write_idx * QUERY_COUNT + i];
        glBeginQuery(query_targets[i], query_handle);
    }

}

static void end_query() {
    printf("end query round robin: %d\n", write_idx);

    for(int i = 0; i < QUERY_COUNT; ++i) {
        glEndQuery(query_targets[i]);
    }

    int next = (write_idx + 1) % ROUNDROBIN_FRAMES;
    if(next != read_idx)
        write_idx = next;
}

static void read_query() {
    if(read_idx == write_idx)
        return;

    printf("reading round robin %d\n", read_idx);
    uint64_t results[QUERY_COUNT];
    int done = 1;
    for(int i = 0; i < QUERY_COUNT; ++i) {
        unsigned int query_handle = query_objects[read_idx * QUERY_COUNT + i];
        int result_available = 0;

        glGetQueryObjectiv(
            query_handle,
            GL_QUERY_RESULT_AVAILABLE,
            &result_available);

        if(!result_available) {
            printf("query busy: %d\n", read_idx);
            done = 0;
            break;
        }

        glGetQueryObjectui64v(
            query_handle,
            GL_QUERY_RESULT,
            &results[i]);
    }

    if(done) {
        printf("query done: %d\n", read_idx);
        for(int i = 0; i < QUERY_COUNT; ++i) {
            printf("%s: %" PRIu64 "\n", query_target_strings[i], results[i]);
        }

        read_idx = (read_idx + 1) % ROUNDROBIN_FRAMES;
    }
}

unsigned int color_buffer, depth_buffer, framebuffer;
int framebuffer_width = 0, framebuffer_height = 0;

static void rendertarget_quit() {
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &color_buffer);
    glDeleteTextures(1, &depth_buffer);
}

static int rendertarget_init(int width, int height, int msaa) {
    printf("resize: %d x %d\n", width, height);

    glGenTextures(1, &color_buffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, color_buffer);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        msaa,
        GL_RGBA8,
        width, height,
        0 /* fixedsamplelocations */);

    glGenTextures(1, &depth_buffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depth_buffer);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        msaa,
        GL_DEPTH24_STENCIL8,
        width, height,
        0 /* fixedsamplelocations */);

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        color_buffer,
        0 /* level */);
    glFramebufferTexture(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        depth_buffer,
        0 /* level */);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
        return -1;

    return 0;
}

static void paint(int width, int height) {
    if(width != framebuffer_width || height != framebuffer_height) {
        //rendertarget_quit();
        rendertarget_init(width, height, 4);

        framebuffer_width = width;
        framebuffer_height = height;
    }

    read_query();

    begin_query();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_MULTISAMPLE);

    const float clear_color[4] = { 0.2, 0.4, 0.7, 1.0 };
    glClearBufferfv(GL_COLOR, 0, clear_color);

    // XXX: glClearBuffer below will cause
    printf("before clear\n");
    float clear_depth = 1.0;
    int clear_stencil = 0;
    glClearBufferfi(GL_DEPTH_STENCIL, 0, clear_depth, clear_stencil);
    printf("after clear\n");


    // multisample resolve blit to default framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(
        0, 0, width, height, /* src */
        0, 0, width, height, /* dst */
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST);

    end_query();
}

static void *get_proc_addr(const char *func, void *ptr) {
    (void)ptr;
    return (void*)glfwGetProcAddress(func);
}

int main(int argc, char *argv) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

    GLFWwindow *window = glfwCreateWindow(100, 100, "querystall", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    gladLoadGL(get_proc_addr, 0);

    glDebugMessageCallback(gl_debug_callback, NULL);

    init();

    while(glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        paint(width, height);

        glfwSwapBuffers(window);
    }
}
