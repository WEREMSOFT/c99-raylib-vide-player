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
    Image video_frame;
    Texture2D video_container_texture;
    AudioStream audio_stream;
    double base_time;
    double frame_rate;
} app_context_t;

app_context_t app_context_create(){
    app_context_t return_value = {0};

    return_value.plm_video = plm_create_with_filename("assets/bjork-all-is-full-of-love.mpg");

    assert(return_value.plm_video && "Can't open video");

    return_value.base_time = GetTime();

    return_value.frame_rate = plm_get_framerate(return_value.plm_video);

    plm_set_loop(return_value.plm_video, true);

    if(plm_get_num_audio_streams(return_value.plm_video) > 0){
        InitAudioDevice();
        int sample_rate = plm_get_samplerate(return_value.plm_video);

        return_value.audio_stream = InitAudioStream(sample_rate, 32, 2);

        PlayAudioStream(return_value.audio_stream);
        plm_set_audio_lead_time(return_value.plm_video, (double)PLM_AUDIO_SAMPLES_PER_FRAME/ (double)sample_rate);
    }

    plm_set_audio_enabled(return_value.plm_video, TRUE);

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
    plm_destroy(app_context->plm_video);
    UnloadImage(app_context->video_frame);
}

void update_frame(void* context)
{
    app_context_t* app_context = (app_context_t*) context;
    static double elapsed_time = 0.0;

    elapsed_time = (GetTime() - app_context->base_time);


    if(elapsed_time >= (1.0 / app_context->frame_rate)){
        app_context->base_time = GetTime();
        plm_decode(app_context->plm_video, elapsed_time);
    }

    BeginDrawing();
    {
        ClearBackground(WHITE);
        DrawTexture(app_context->video_container_texture, 0, 0, WHITE);
        DrawFPS(10, 10);
    }
    EndDrawing();

}

void app_on_audio(plm_t *mpeg, plm_samples_t *samples, void *user) {
	app_context_t *app_context = (app_context_t *)user;
    
    UpdateAudioStream(app_context->audio_stream, samples->interleaved, PLM_AUDIO_SAMPLES_PER_FRAME * 2);
}

void app_on_video(plm_t *mpeg, plm_frame_t *frame, void *user) {
	app_context_t *app_context = (app_context_t *)user;

    static int stride = 2880;
 
    plm_frame_to_rgb(frame, app_context->video_frame.data, stride);

    UpdateTexture(app_context->video_container_texture, app_context->video_frame.data);
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


    InitWindow(WIDTH, HEIGHT, "This is a video decoding test");
    SetTargetFPS(100);
   
    app_context_t app_context = app_context_create();

    plm_set_video_decode_callback(app_context.plm_video, app_on_video, &app_context);
    plm_set_audio_decode_callback(app_context.plm_video, app_on_audio, &app_context);

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