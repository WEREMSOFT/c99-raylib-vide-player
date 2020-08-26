#include <stdio.h>
#include <raylib.h>
#include <assert.h>

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>

#ifdef OS_WEB
#include <emscripten/emscripten.h>
#endif

#define WIDTH 800
#define HEIGHT 600

typedef struct app_context_t {
    plm_t* plm_video;
    RenderTexture2D canvas;
    Image video_frame;
    Texture2D video_container_texture;
    double seek_to;
} app_context_t;


void app_on_video(plm_t *mpeg, plm_frame_t *frame, void *user) {
	app_context_t *app_context = (app_context_t *)user;

    static int stride = 2880;
 
    plm_frame_to_rgb(frame, app_context->video_frame.data, stride);

    UpdateTexture(app_context->video_container_texture, app_context->video_frame.data);
    
    BeginDrawing();
    {

        ClearBackground(WHITE);
        DrawTexture(app_context->video_container_texture, 0, 0, WHITE);
        DrawFPS(10, 10);

    }
    EndDrawing();

}

app_context_t app_context_create(){
    app_context_t return_value = {0};

    return_value.canvas = LoadRenderTexture(WIDTH, HEIGHT);
    return_value.plm_video = plm_create_with_filename("assets/bjork-all-is-full-of-love.mpg");

    assert(return_value.plm_video && "Can't open video");

    plm_set_loop(return_value.plm_video, true);

    printf("framerrate: %f\n", plm_get_framerate(return_value.plm_video));

    int num_pixels = plm_get_width(return_value.plm_video) * plm_get_height(return_value.plm_video);
	uint8_t *rgb_data = (uint8_t*)malloc(num_pixels * 3);

    return_value.video_frame.data = rgb_data;
    return_value.video_frame.width = plm_get_width(return_value.plm_video);
    return_value.video_frame.height = plm_get_height(return_value.plm_video);
    return_value.video_frame.format = UNCOMPRESSED_R8G8B8;
    return_value.video_frame.mipmaps = 1;

    return_value.video_container_texture = LoadTextureFromImage(return_value.video_frame);

    return return_value;
}

void app_context_fini(app_context_t* app_context){
    UnloadRenderTexture(app_context->canvas);
    plm_destroy(app_context->plm_video);
    UnloadImage(app_context->video_frame);
}

void update_frame(app_context_t* app_context)
{
    static int stride = 2880;
    static double elapsed_time = 0.0;
    static double elapsed_time_increment = 0.001;

    elapsed_time = GetTime();

    plm_decode(app_context->plm_video, elapsed_time);
}

int main(void)
{
#ifdef OS_Windows_NT
    printf("Windows dettected\n");
#elif defined OS_Linux
    printf("LINUS dettected\n");
#elif defined OS_Darwin
    printf("MacOS dettected\n");
#endif


    InitWindow(WIDTH, HEIGHT, "This is a network test");
    SetTargetFPS(100);
   
    app_context_t app_context = app_context_create();

    plm_set_video_decode_callback(app_context.plm_video, app_on_video, &app_context);

    BeginTextureMode(app_context.canvas);
    DrawCircle(10, 10, 10, GREEN);
    EndTextureMode();

#ifdef OS_WEB
    emscripten_set_main_loop_arg(update_frame, &app_context, 0, 1);
#else
    while (!WindowShouldClose())
    {
        update_frame(&app_context);
    }
#endif
    app_context_fini(&app_context);
    CloseWindow();

    return 0;
}