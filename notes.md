
#include "raylib.h"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

enum app_state_enum {
    APP_STATE_WAITING_FOR_INTERACTION,
    APP_STATE_PLAYING_VIDEO,
};

typedef struct app_context_t {
    unsigned int state;
    plm_t* plm_video;
    Image video_frame;
    Texture2D video_container_texture;
    AudioStream audio_stream;
    double base_time;
    double last_time;
    double frame_rate;
    plm_samples_t* samples;
} app_context_t;

app_context_t app_context_create(const char *fileName)
{
    app_context_t return_value = {0};

    return_value.plm_video = plm_create_with_filename(fileName);

    return_value.last_time = return_value.base_time = GetTime();

    return_value.frame_rate = plm_get_framerate(return_value.plm_video);

    plm_set_loop(return_value.plm_video, true);

    if (plm_get_num_audio_streams(return_value.plm_video) > 0)
    {
        InitAudioDevice();
        int sample_rate = plm_get_samplerate(return_value.plm_video);

        SetAudioStreamBufferSizeDefault(1152);
        return_value.audio_stream = InitAudioStream(sample_rate, 32, 2);

        PlayAudioStream(return_value.audio_stream);
        //plm_set_audio_lead_time(return_value.plm_video, (double)PLM_AUDIO_SAMPLES_PER_FRAME/(double)sample_rate);
    }

	plm_set_audio_stream(return_value.plm_video, 0);

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

void app_context_fini(app_context_t* app_context)
{
    plm_destroy(app_context->plm_video);
    UnloadImage(app_context->video_frame);
    CloseAudioStream(app_context->audio_stream);
    CloseAudioDevice();
}

static double elapsed_time = 0.0;

void UpdateDrawFrame(void* context)
{
    app_context_t* app_context = (app_context_t*)context;

    if (app_context->state == APP_STATE_WAITING_FOR_INTERACTION)
    {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_A))
        {
            *app_context = app_context_create("resources/bjork-all-is-full-of-love.mpg");
            app_context->state = APP_STATE_PLAYING_VIDEO;
        }
    }
    else if (app_context->state == APP_STATE_PLAYING_VIDEO)
    {
        elapsed_time = (GetTime() - app_context->last_time);

        if (elapsed_time >= (1.0/app_context->frame_rate))
        {
            app_context->last_time = GetTime();
            
            plm_video_set_time(app_context->plm_video->video_decoder, app_context->last_time - app_context->base_time);
            plm_frame_t *frame = plm_decode_video(app_context->plm_video);
            plm_frame_to_rgb(frame, app_context->video_frame.data, app_context->video_container_texture.width*3);
            
            UpdateTexture(app_context->video_container_texture, app_context->video_frame.data);
        }

        while (IsAudioStreamProcessed(app_context->audio_stream))
        {
            app_context->samples = plm_decode_audio(app_context->plm_video);
            UpdateAudioStream(app_context->audio_stream, app_context->samples->interleaved, PLM_AUDIO_SAMPLES_PER_FRAME*2);    
        }
    }

    BeginDrawing();

        ClearBackground(WHITE);
        
        if (app_context->state == APP_STATE_WAITING_FOR_INTERACTION) DrawText("CLICK TO START THE VIDEO", 330, 200, 20, LIGHTGRAY);
        else DrawTexture(app_context->video_container_texture, 0, 0, WHITE);
        
        DrawFPS(10, 10);

    EndDrawing();
}

int main(void)
{
    InitWindow(960, 540, "This is a video decoding test");
    SetTargetFPS(100);
   
    app_context_t app_context = {0};

#ifdef PLATFORM_WEB
    emscripten_set_main_loop_arg(UpdateDrawFrame, &app_context, 0, 1);
#else
    while (!WindowShouldClose()) UpdateDrawFrame(&app_context);
#endif

    app_context_fini(&app_context);
    CloseWindow();

    return 0;
}