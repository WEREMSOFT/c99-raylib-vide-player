#include <stdio.h>
#include <raylib.h>
#include <assert.h>

#include "app_context.h"

#ifdef OS_WEB
#include <emscripten/emscripten.h>
#endif


#define WIDTH 960
#define HEIGHT 540

void update_frame(void* context)
{
    static double elapsed_time = 0.0;
    app_context_t* app_context = (app_context_t*) context;

    switch (app_context->state)
    {
    case APP_STATE_WAITING_FOR_INTERACTION:
        BeginDrawing();
        {
            ClearBackground(WHITE);
            DrawText("CLICK TO START THE VIDEO", 330, 200, 20, LIGHTGRAY);
            DrawFPS(10, 10);
        }
        EndDrawing();
        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_A)) {
            *app_context = app_context_create();
            app_context->state = APP_STATE_PLAYING_VIDEO;
            printf("CLICKED!!\n");
        }
        break;
    case APP_STATE_PLAYING_VIDEO:
        elapsed_time = (GetTime() - app_context->last_time);

        if(elapsed_time >= app_context->frame_time){
            app_context->last_time = GetTime();
            
            plm_frame_t *frame = plm_decode_video(app_context->plm_video);
    
            plm_frame_to_rgb(frame, app_context->video_frame.data, app_context->video_vertical_line_size_in_bytes);
            UpdateTexture(app_context->video_container_texture, app_context->video_frame.data);
            BeginDrawing();
            {
                ClearBackground(WHITE);
                DrawTexture(app_context->video_container_texture, 0, 0, WHITE);
                DrawFPS(10, 10);
            }
            EndDrawing();
        }

        while(IsAudioStreamProcessed(app_context->audio_stream)){
            app_context->samples = plm_decode_audio(app_context->plm_video);
            UpdateAudioStream(app_context->audio_stream, app_context->samples->interleaved, PLM_AUDIO_SAMPLES_PER_FRAME * 2);    
        }
        break;
    default:
        break;
    }
    

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
   
    app_context_t app_context = {0};

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