#ifndef __APP_CONTEXT_H__
#define __APP_CONTEXT_H__
    #ifndef PL_MPEG_IMPLEMENTATION
        #define PL_MPEG_IMPLEMENTATION
    #endif
#include <pl_mpeg.h>
#include <raylib.h>

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
    double frame_time;
    unsigned int video_vertical_line_size_in_bytes;
    plm_samples_t* samples;
} app_context_t;

app_context_t app_context_create(){
    app_context_t return_value = {0};

    return_value.plm_video = plm_create_with_filename("assets/bjork-all-is-full-of-love.mpg");

    assert(return_value.plm_video && "Can't open video");

    return_value.last_time = return_value.base_time = GetTime();

    return_value.frame_rate = plm_get_framerate(return_value.plm_video);
    return_value.frame_time = 1.0f / return_value.frame_rate;

    plm_set_loop(return_value.plm_video, true);

    if(plm_get_num_audio_streams(return_value.plm_video) > 0){
        InitAudioDevice();
        int sample_rate = plm_get_samplerate(return_value.plm_video);

        SetAudioStreamBufferSizeDefault(1152);
        return_value.audio_stream = InitAudioStream(sample_rate, 32, 2);

        PlayAudioStream(return_value.audio_stream);
        plm_set_audio_lead_time(return_value.plm_video, (double)PLM_AUDIO_SAMPLES_PER_FRAME/ (double)sample_rate);
    }

	plm_set_audio_stream(return_value.plm_video, 0);

    printf("video size: %d %d", plm_get_width(return_value.plm_video), plm_get_height(return_value.plm_video));

    return_value.video_vertical_line_size_in_bytes = plm_get_width(return_value.plm_video) * 3;

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
    CloseAudioStream(app_context->audio_stream);
    CloseAudioDevice();
}


#endif
