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
    
    
//    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
//        float delta_time = ticks - g_previous_ticks;
//        g_previous_ticks = ticks;
//
//        g_frame_counter += 1;
//
//        // Update for scaling behavior
//        if (g_frame_counter >= G_MAX_FRAME)
//        {
//            g_is_growing = !g_is_growing;
//            g_frame_counter = 0;
//        }
//
//        glm::vec3 scale_vector = glm::vec3(
//            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//            1.0f
//        );
//
//        // Step 2: Update Totsuko's circular motion around Kimi
//        g_totsuko_angle += 0.1f * delta_time; // Increment angle based on delta time
//
//        // Create transformation matrix for Totsuko
//        g_totsuko_matrix = glm::mat4(1.0f);
//
//        // Step 3: Translate Totsuko to Kimi's position
//        g_totsuko_matrix = glm::translate(g_totsuko_matrix, INIT_POS_KIMI); // Move to Kimi's position
//
//    // Step 5: Translate outward to place Totsuko at the desired radius from Kimi
//    g_totsuko_matrix = glm::translate(g_totsuko_matrix, glm::vec3(CIRCLE_RADIUS, 0.0f, 0.0f)); // Move outward by CIRCLE_RADIUS
//    
//        // Step 4: Rotate Totsuko around Kimi's position
//        g_totsuko_matrix = glm::rotate(g_totsuko_matrix, g_totsuko_angle, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around the Z-axis
//
//    // Step 1: Update Kimi's rotation and scaling
//    g_kimi_matrix = glm::rotate(g_kimi_matrix, ROT_ANGLE * delta_time, glm::vec3(0.0f, 0.0f, 1.0f));
//
//        // Step 6: Scale Totsuko
//        g_totsuko_matrix = glm::scale(g_totsuko_matrix, TOTSUKO_SCALE); // Scale Totsuko
//    
//    g_kimi_matrix = glm::scale(g_kimi_matrix, scale_vector);
//  xxxxxx
//    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
//        float delta_time = ticks - g_previous_ticks;
//        g_previous_ticks = ticks;
//
//        g_frame_counter += 1;
//
//        // Apply rotation to Kimi using delta time
//        g_kimi_matrix = glm::rotate(g_kimi_matrix, ROT_ANGLE * delta_time, glm::vec3(0.0f, 0.0f, 1.0f));
//
//        // Update for scaling behavior
//        if (g_frame_counter >= G_MAX_FRAME)
//        {
//            g_is_growing = !g_is_growing;
//            g_frame_counter = 0;
//        }
//
//        glm::vec3 scale_vector = glm::vec3(
//            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//            1.0f
//        );
//    // Step 1: Translate to Kimi's position
//    g_totsuko_matrix = glm::translate(g_totsuko_matrix, INIT_POS_KIMI); // Move to Kimi's position
//    
//    g_totsuko_matrix = glm::translate(g_kimi_matrix, glm::vec3(CIRCLE_RADIUS, 0.0f, 0.0f)); // Move outward by CIRCLE_RADIUS
//    
//    // Apply rotation to Kimi using delta time
//    g_kimi_matrix = glm::rotate(g_kimi_matrix, ROT_ANGLE * delta_time, glm::vec3(0.0f, 0.0f, 1.0f));
//        
//    g_totsuko_matrix = glm::rotate(g_totsuko_matrix, g_totsuko_angle, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around the Z-axis
//    
//    g_kimi_matrix = glm::scale(g_kimi_matrix, scale_vector);
//    
//    // Scale Totsuko
//    g_totsuko_matrix = glm::scale(g_totsuko_matrix, TOTSUKO_SCALE); // Scale Totsuko
//
//        // Update Totsuko's circular motion around Kimi
//        g_totsuko_angle += 0.1f * delta_time; // Increment angle based on delta time
//
//        // Create transformation matrix for Totsuko
//        g_totsuko_matrix = glm::mat4(1.0f);

        


        // Step 3: Translate outward to place Totsuko at the desired radius from Kimi
        

     
    
//
//    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
//        float delta_time = ticks - g_previous_ticks;
//        g_previous_ticks = ticks;
//
//        g_frame_counter += 1;
//
//        // Apply rotation using delta time
//        g_kimi_matrix = glm::rotate(g_kimi_matrix, ROT_ANGLE * delta_time, glm::vec3(0.0f, 0.0f, 1.0f));
//
//        // Update for scaling behavior
//        if (g_frame_counter >= G_MAX_FRAME)
//        {
//            g_is_growing = !g_is_growing;
//            g_frame_counter = 0;
//        }
//
//        glm::vec3 scale_vector = glm::vec3(
//            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//            g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//            1.0f);
//
//        g_kimi_matrix = glm::scale(g_kimi_matrix, scale_vector);
//
//        // Circular motion for Totsuko
//        g_totsuko_angle += 0.1f * delta_time; // Increment angle based on delta time
//        float x_position = CIRCLE_RADIUS * cos(g_totsuko_angle);
//        float y_position = CIRCLE_RADIUS * sin(g_totsuko_angle);
//
//        // Create transformation matrix for Totsuko
//        g_totsuko_matrix = glm::mat4(1.0f);
//        g_totsuko_matrix = glm::translate(g_totsuko_matrix, glm::vec3(x_position, y_position, 0.0f));
//        g_totsuko_matrix = glm::scale(g_totsuko_matrix, TOTSUKO_SCALE);
//    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
//    float delta_time = ticks - g_previous_ticks;
//    g_previous_ticks = ticks;
//
//    g_frame_counter += 1;
//
//    // Apply rotation using delta time
//    g_kimi_matrix = glm::rotate(g_kimi_matrix, ROT_ANGLE * delta_time, glm::vec3(0.0f, 0.0f, 1.0f));
//    
//    // Update for scaling behavior
//    if (g_frame_counter >= G_MAX_FRAME)
//    {
//        g_is_growing = !g_is_growing;
//        g_frame_counter = 0;
//    }
//    
//    glm::vec3 scale_vector = glm::vec3(
//        g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//        g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
//        1.0f);
//    
//    g_kimi_matrix = glm::scale(g_kimi_matrix, scale_vector);
//
//    // Circular motion for Totsuko
//    g_totsuko_angle += 0.1f * delta_time; // Increment angle based on delta time
//    float x_position = CIRCLE_RADIUS * cos(g_totsuko_angle);
//    float y_position = CIRCLE_RADIUS * sin(g_totsuko_angle);
//
//    // Create transformation matrix for Totsuko
//    g_totsuko_matrix = glm::mat4(1.0f);
//    g_totsuko_matrix = glm::translate(g_totsuko_matrix, glm::vec3(x_position, y_position, 0.0f));
//    g_totsuko_matrix = glm::scale(g_totsuko_matrix, INIT_SCALE);
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








//#define GL_SILENCE_DEPRECATION
//#define STB_IMAGE_IMPLEMENTATION
//#define GL_GLEXT_PROTOTYPES 1
//#define LOG(argument) std::cout << argument << '\n'
//
//#ifdef _WINDOWS
//    #include <GL/glew.h>
//#endif
//
//#include <SDL.h>
//#include <SDL_opengl.h>
//#include "glm/mat4x4.hpp"
//#include "glm/gtc/matrix_transform.hpp"
//#include "ShaderProgram.h"
//#include "stb_image.h"
//
//enum AppStatus { RUNNING, TERMINATED };
//
//constexpr int WINDOW_WIDTH  = 640 * 2,
//              WINDOW_HEIGHT = 480 * 2;
//
//constexpr float BG_RED     = 0.9765625f,
//                BG_GREEN   = 0.97265625f,
//                BG_BLUE    = 0.9609375f,
//                BG_OPACITY = 1.0f;
//
//constexpr int VIEWPORT_X      = 0,
//              VIEWPORT_Y      = 0,
//              VIEWPORT_WIDTH  = WINDOW_WIDTH,
//              VIEWPORT_HEIGHT = WINDOW_HEIGHT;
//
//constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
//               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";
//
//constexpr float MILLISECONDS_IN_SECOND = 1000.0f;
//
//constexpr GLint NUMBER_OF_TEXTURES = 1,
//                LEVEL_OF_DETAIL    = 0,
//                TEXTURE_BORDER     = 0;
//
//constexpr char SHIELD_SPRITE_FILEPATH[] = "shield.png";
//constexpr glm::vec3 INIT_SCALE = glm::vec3(2.0f, 2.3985f, 0.0f);
//
//SDL_Window* g_display_window = nullptr;
//AppStatus g_app_status = RUNNING;
//
//ShaderProgram g_shader_program = ShaderProgram();
//
//GLuint g_shield_texture_id;
//
//glm::mat4 g_view_matrix,
//          g_shield_matrix,
//          g_projection_matrix;
//
//float g_previous_ticks = 0.0f;
//
//glm::vec3 g_shield_position = glm::vec3(0.0f, 0.0f, 0.0f);
//glm::vec3 g_shield_movement = glm::vec3(0.0f, 0.0f, 0.0f);
//
//float g_shield_speed = 2.0f;  // move 2 units per second
//
//void initialise();
//void process_input();
//void update();
//void render();
//void shutdown();
//
//GLuint load_texture(const char* filepath);
//void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id);
//
//
//GLuint load_texture(const char* filepath)
//{
//    // STEP 1: Loading the image file
//    int width, height, number_of_components;
//    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components,
//                                     STBI_rgb_alpha);
//
//    if (image == NULL)
//    {
//        LOG("Unable to load image. Make sure the path is correct.");
//        assert(false);
//    }
//
//    // STEP 2: Generating and binding a texture ID to our image
//    GLuint textureID;
//    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
//    glBindTexture(GL_TEXTURE_2D, textureID);
//    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
//                 GL_RGBA, GL_UNSIGNED_BYTE, image);
//
//    // STEP 3: Setting our texture filter parameters
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//
//    // STEP 4: Releasing our file from memory and returning our texture id
//    stbi_image_free(image);
//
//    return textureID;
//}
//
//
//void initialise()
//{
//    SDL_Init(SDL_INIT_VIDEO);
//    g_display_window = SDL_CreateWindow("Hello, Playa Input!",
//                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
//                                      WINDOW_WIDTH, WINDOW_HEIGHT,
//                                      SDL_WINDOW_OPENGL);
//
//    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
//    SDL_GL_MakeCurrent(g_display_window, context);
//
//    if (g_display_window == nullptr)
//    {
//        shutdown();
//    }
//
//#ifdef _WINDOWS
//    glewInit();
//#endif
//
//    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
//
//    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
//
//    g_shield_texture_id = load_texture(SHIELD_SPRITE_FILEPATH);
//    g_shield_matrix     = glm::mat4(1.0f);
//    g_view_matrix       = glm::mat4(1.0f);
//    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
//
//    g_shader_program.set_projection_matrix(g_projection_matrix);
//    g_shader_program.set_view_matrix(g_view_matrix);
//
//    glUseProgram(g_shader_program.get_program_id());
//    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//}
//
//
//void process_input()
//{
//    g_shield_movement = glm::vec3(0.0f);
//    
//    SDL_Event event;
//    while (SDL_PollEvent(&event))
//    {
//        switch (event.type) 
//        {
//            case SDL_QUIT:
//            case SDL_WINDOWEVENT_CLOSE:
//                g_app_status = TERMINATED;
//                break;
//                
//            case SDL_KEYDOWN:
//                switch (event.key.keysym.sym) 
//                {
//                    case SDLK_q:
//                        g_app_status = TERMINATED;
//                        break;
//                    
////                    case SDLK_a:
////                        g_shield_movement.x = -1.0f;
//                        
//                    default:
//                        break;
//                }
//            default:
//                break;
//        }
//    }
//    
//    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
//    
//    if (key_state[SDL_SCANCODE_LEFT])
//    {
//        g_shield_movement.x = -1.0f;
//    } 
//    else if (key_state[SDL_SCANCODE_RIGHT])
//    {
//        g_shield_movement.x = 1.0f;
//    }
//    
//    if (key_state[SDL_SCANCODE_UP])
//    {
//        g_shield_movement.y = 1.0f;
//    }
//    else if (key_state[SDL_SCANCODE_DOWN])
//    {
//        g_shield_movement.y = -1.0f;
//    }
//    
//    if (glm::length(g_shield_movement) > 1.0f)
//        g_shield_movement = glm::normalize(g_shield_movement);
//}
//
//
//void update()
//{
//    /* DELTA TIME */
//    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
//    float delta_time = ticks - g_previous_ticks;
//    g_previous_ticks = ticks;
//
//    /* GAME LOGIC */
//    g_shield_position += g_shield_movement * g_shield_speed * delta_time;
//
//    /* TRANSFORMATIONS */
//    g_shield_matrix = glm::mat4(1.0f);
//
//    g_shield_matrix = glm::translate(g_shield_matrix, g_shield_position);
//    g_shield_matrix = glm::scale(g_shield_matrix, INIT_SCALE);
//}
//
//
//void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
//{
//    g_shader_program.set_model_matrix(object_model_matrix);
//    glBindTexture(GL_TEXTURE_2D, object_texture_id);
//    glDrawArrays(GL_TRIANGLES, 0, 6);
//}
//
//
//void render()
//{
//    glClear(GL_COLOR_BUFFER_BIT);
//
//    float vertices[] = {
//        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
//        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
//    };
//
//    // Textures
//    float texture_coordinates[] = {
//        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
//        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
//    };
//
//    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
//                          0, vertices);
//    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
//
//    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
//                          false, 0, texture_coordinates);
//    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
//
//    // Bind texture
//    draw_object(g_shield_matrix, g_shield_texture_id);
//
//    // We disable two attribute arrays now
//    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
//    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
//
//    SDL_GL_SwapWindow(g_display_window);
//}
//
//
//void shutdown() { SDL_Quit(); }
//
//
//int main(int argc, char* argv[])
//{
//    initialise();
//
//    while (g_app_status == RUNNING)
//    {
//        process_input();
//        update();
//        render();
//    }
//
//    shutdown();
//    return 0;
//}
