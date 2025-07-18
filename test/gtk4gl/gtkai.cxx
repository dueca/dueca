// File: gtk4_modern_opengl_triangle.cpp
#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <iostream>
#include <string>

GLuint vao = 0, vbo = 0, shader_program = 0;

const char* vertex_shader_src = R"glsl(
    #version 330 core
    layout(location = 0) in vec2 position;
    layout(location = 1) in vec3 color;
    out vec3 fragColor;
    void main() {
        fragColor = color;
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";

const char* fragment_shader_src = R"glsl(
    #version 330 core
    in vec3 fragColor;
    out vec4 outColor;
    void main() {
        outColor = vec4(fragColor, 1.0);
    }
)glsl";

// Compile shader and check for errors
GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        std::cerr << "Shader compile error: " << info << std::endl;
    }

    return shader;
}

static void on_realize(GtkGLArea* area, gpointer) {
    gtk_gl_area_make_current(area);
    if (gtk_gl_area_get_error(area)) {
        std::cerr << "Failed to create GL context" << std::endl;
        return;
    }

    // Compile and link shaders
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vs);
    glAttachShader(shader_program, fs);
    glLinkProgram(shader_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Triangle vertices: position (x, y) + color (r, g, b)
    float vertices[] = {
         0.0f,  0.6f,   1.0f, 0.0f, 0.0f,
        -0.6f, -0.6f,   0.0f, 1.0f, 0.0f,
         0.6f, -0.6f,   0.0f, 0.0f, 1.0f
    };

    // VAO and VBO setup
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static gboolean on_render(GtkGLArea* area, GdkGLContext* context, gpointer) {
    std::cerr << "render" << std::endl;
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    return TRUE;
}

static void on_activate(GApplication* app, gpointer) {
    GtkWidget* window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "Modern OpenGL Triangle (GTK4)");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    GtkWidget* gl_area = gtk_gl_area_new();
    gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA(gl_area), FALSE);
    gtk_gl_area_set_required_version(GTK_GL_AREA(gl_area), 3, 3); // Ensure core profile
    gtk_widget_set_hexpand(gl_area, TRUE);
    gtk_widget_set_vexpand(gl_area, TRUE);

    g_signal_connect(gl_area, "realize", G_CALLBACK(on_realize), NULL);
    g_signal_connect(gl_area, "render", G_CALLBACK(on_render), NULL);

    gtk_window_set_child(GTK_WINDOW(window), gl_area);
    // gtk_widget_show(window);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new("com.example.ModernGL", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
