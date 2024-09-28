/**
 * @file main.cpp
 * @author Sebastián Romero Cruz (sebastian.romerocruz@nyu.edu)
 * @brief A simple g_shader_program to demonstrate player input in OpenGL.
 * @date 2024-06-10
 *
 * @copyright NYU Tandon (c) 2024
 */
/**
* Author: Avyansh Gupta
* Assignment: Simple 2D Scene
* Date due: 2023-09-20, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL2/SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
                LEVEL_OF_DETAIL    = 0, // mipmap reduction image level
                TEXTURE_BORDER     = 0; // this value MUST be zero

// Make sure the paths are correct on your system
constexpr char KIMI_SPRITE_FILEPATH[]    = "/Users/avyanshgupta/Desktop/kimi.png",
               TOTSUKO_SPRITE_FILEPATH[] = "/Users/avyanshgupta/Desktop/totsuko.png";

constexpr glm::vec3 INIT_SCALE       = glm::vec3(5.0f, 5.98f, 0.0f),
                    INIT_POS_KIMI    = glm::vec3(2.0f, 0.0f, 0.0f),
                    INIT_POS_TOTSUKO = glm::vec3(-2.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT     = 1.0f;
constexpr float TRAN_VALUE        = 0.025f;
constexpr float G_GROWTH_FACTOR   = 1.01f;
constexpr float G_SHRINK_FACTOR   = 0.99f;
constexpr float ROT_ANGLE         = glm::radians(1.5f); // Smaller rotation angle
constexpr int   G_MAX_FRAME       = 40;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_kimi_matrix,
          g_totsuko_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;
int g_frame_counter = 0;
bool g_is_growing = true;

GLuint g_kimi_texture_id,
       g_totsuko_texture_id;

constexpr float CIRCLE_RADIUS = 2.0f; // Radius for circular motion
float g_totsuko_angle = 0.0f; // Angle for circular motion

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Hello, Transformations!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_kimi_matrix       = glm::mat4(1.0f); // Start upright, no initial rotation
    g_totsuko_matrix    = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_kimi_texture_id   = load_texture(KIMI_SPRITE_FILEPATH);
    g_totsuko_texture_id = load_texture(TOTSUKO_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}
constexpr glm::vec3 TOTSUKO_SCALE = glm::vec3(0.99f, 0.99f, 1.0f);
constexpr glm::vec3 KIMI_SCALE = glm::vec3(1.0f, 1.0f, 1.0f); // Fixed scale for Kimi
constexpr float ORBIT_SPEED = 1.0f; // Adjust this for speed of orbit
constexpr float RADIUS = 2.0f;       // Distance from Kimi to Totsuko

float g_angle = 0.0f; // Angle for Totsuko's orbit
float g_x_offset = 0.0f; // X offset for Totsuko's position
float g_y_offset = 0.0f; // Y offset for Totsuko's position




void update()
{
    
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
        float delta_time = ticks - g_previous_ticks;
        g_previous_ticks = ticks;

        // Step 1: Update for Kimi's scaling behavior (pumping effect)
        g_frame_counter += 1;

        // Update for scaling behavior (pumping effect)
        if (g_frame_counter >= G_MAX_FRAME)
        {
            g_is_growing = !g_is_growing;
            g_frame_counter = 0;
        }

        glm::vec3 scale_vector = glm::vec3(
            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
            1.0f);

        // Create transformation matrix for Kimi (no translation or rotation)
        g_kimi_matrix = glm::mat4(1.0f); // Reset model matrix for Kimi
        g_kimi_matrix = glm::scale(g_kimi_matrix, scale_vector * KIMI_SCALE); // Apply scaling effect

        // Step 2: Update Totsuko's angle for orbit
        g_angle += ORBIT_SPEED * delta_time; // Increment angle for orbit

        // Step 3: Calculate new x, y position using trigonometry for Totsuko's orbit
        g_x_offset = RADIUS * glm::cos(g_angle);
        g_y_offset = RADIUS * glm::sin(g_angle);

        // Step 4: Update Totsuko's transformation matrix
        g_totsuko_matrix = glm::mat4(1.0f); // Reset model matrix for Totsuko
        g_totsuko_matrix = glm::translate(g_kimi_matrix, glm::vec3(g_x_offset, g_y_offset, 0.0f)); // Orbit around Kimi
        g_totsuko_matrix = glm::scale(g_totsuko_matrix, TOTSUKO_SCALE); // Scale Totsuko
    
    

}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Drawing the two triangles for each object
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_kimi_matrix, g_kimi_texture_id);
    draw_object(g_totsuko_matrix, g_totsuko_texture_id);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    SDL_Quit();
    return 0;
}





