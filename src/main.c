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
    double seek_to;
	uint8_t *rgb_data;
} app_context_t;


void app_on_video(plm_t *mpeg, plm_frame_t *frame, void *user) {
	app_context_t *self = (app_context_t *)user;
	
	// Hand the decoded data over to OpenGL. For the RGB texture mode, the
	// YCrCb->RGB conversion is done on the CPU.

    Image checkedIm = {
        .data = self->rgb_data,             // We can assign pixels directly to data
        .width = frame->width,
        .height = frame->height,
        .format = UNCOMPRESSED_R8G8B8,
        .mipmaps = 1
    }; 

    printf("frame decoded\n");
	plm_frame_to_rgb(frame, self->rgb_data, frame->width * 3);

    Texture2D temp_texture = LoadTextureFromImage(checkedIm);
    UnloadImage(checkedIm);

    BeginTextureMode(self->canvas);
    DrawTexture(temp_texture, 0, 0, WHITE);
    EndTextureMode();

    UnloadTexture(temp_texture);

}

app_context_t app_context_create(){
    app_context_t return_value = {0};

    return_value.canvas = LoadRenderTexture(WIDTH, HEIGHT);
    return_value.plm_video = plm_create_with_filename("assets/bjork-all-is-full-of-love.mpg");

    plm_set_video_decode_callback(return_value.plm_video, app_on_video, &return_value);

    assert(return_value.plm_video && "Can't open video");

    int num_pixels = plm_get_width(return_value.plm_video) * plm_get_height(return_value.plm_video);
	return_value.rgb_data = (uint8_t*)malloc(num_pixels * 3);

    return_value.seek_to = plm_get_duration(return_value.plm_video) * 0.5f;
    plm_seek(return_value.plm_video, return_value.seek_to, FALSE);


    Image checked = GenImageChecked(100, 100, 50, 50, RED, GREEN);

    Texture2D texture = LoadTextureFromImage(checked);
    UnloadImage(checked);

    BeginTextureMode(return_value.canvas);
    DrawTexture(texture, 100, 100, WHITE);
    EndTextureMode();

    return return_value;
}

void app_context_fini(app_context_t* app_context){
    UnloadRenderTexture(app_context->canvas);
    plm_destroy(app_context->plm_video);
    free(app_context->rgb_data);
}

void update_frame(app_context_t* app_context)
{
    BeginDrawing();
    {

        ClearBackground(WHITE);
        DrawTextureRec(app_context->canvas.texture, (Rectangle){0, 0, WIDTH, -HEIGHT}, (Vector2){0}, WHITE);
        DrawFPS(10, 10);

    }
    EndDrawing();
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
    SetTargetFPS(60);
   
    app_context_t app_context = app_context_create();

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