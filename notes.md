/*******************************************************************************************
*
*   raylib [textures] example - MPEG video playing
*
*   We have two options to decode video & audio using pl_mpeg.h library:
*
*   1) Use plm_decode() and just hand over the delta time since the last call.
*      It will decode everything needed and call your callbacks (specified through
*      plm_set_{video|audio}_decode_callback()) any number of times.
*
*   2) Use plm_decode_video() and plm_decode_audio() to decode exactly one
*      frame of video or audio data at a time. How you handle the synchronization of
*     both streams is up to you.
*
*   This example uses option 2)
*
*   This example has been created using raylib 3.0 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2020 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <stdlib.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

int main(void)
{
    // Initialization
    //---------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "raylib [textures] example - MPEG video playing");
    
    AudioStream stream = { 0 };

    plm_t *plm = plm_create_with_filename("bjork-all-is-full-of-love.mpg");

    if (!plm) return 1;
    
    double framerate = plm_get_framerate(plm);
    int samplerate = plm_get_samplerate(plm);

	TraceLog(LOG_INFO, "Framerate: %f, samplerate: %d",	(float)framerate, samplerate);
       
	if (plm_get_num_audio_streams(plm) > 0) 
    {
        InitAudioDevice();

        // Init raw audio stream (sample rate: 44100, sample size: 32bit, channels: 2-stereo)
        // WARNING: InitAudioDevice() inits internal double buffering system to AUDIO_BUFFER_SIZE*2,
        // but every audio sample is PLM_AUDIO_SAMPLES_PER_FRAME, usually 1152 samples...
        // Two solutions:
        // 1. Just change raudio internal AUDIO_BUFFER_SIZE to match PLM_AUDIO_SAMPLES_PER_FRAME (1152)
        // 2. Keep internal raudio AUDIO_BUFFER_SIZE (4096) and fill it with multiple plm audio samples,
        //    main issue is that (4096/1152 = 3.555) no round numbers, so, some samples should be divided
        //    into the double buffering system... not trivial to do...
        stream = InitAudioStream(samplerate, 32, 2);

        PlayAudioStream(stream);        // Start processing stream buffer (no data loaded currently)

		// Adjust the audio lead time according to the audio_spec buffer size
		plm_set_audio_lead_time(plm, (double)PLM_AUDIO_SAMPLES_PER_FRAME/(double)samplerate);
	}
	
	plm_set_loop(plm, TRUE);
	plm_set_audio_enabled(plm, TRUE, 0);
	
	int width = plm_get_width(plm);
	int height = plm_get_height(plm);
    
	plm_frame_t *frame = NULL;
    plm_samples_t *sample = NULL;
    
    Image imFrame = { 0 };
    imFrame.width = width;
    imFrame.height = height;
    imFrame.format = UNCOMPRESSED_R8G8B8;
    imFrame.mipmaps = 1;
    imFrame.data = (unsigned char *)malloc(width*height*3);

    Texture texture = LoadTextureFromImage(imFrame);
    
    bool pause = false;
    int framesCounter = 0;
    
    double baseTime = GetTime();    // Time since InitWindow()

    //SetTargetFPS(100);              // Set our game to run at 100 frames-per-second
    //----------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //-----------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) pause = !pause;

        if (!pause)
        {
            // Video should run at 'framerate' fps => One new frame every 1/framerate
            double time = (GetTime() - baseTime);
            
            if (time >= (1.0/framerate))
            {
                baseTime = GetTime();
                
                // Decode video frame
                frame = plm_decode_video(plm);          // Get frame as 3 planes: Y, Cr, Cb
                plm_frame_to_rgb(frame, imFrame.data);  // Convert (Y, Cr, Cb) to RGB on the CPU (slow)
                
                // Update texture
                UpdateTexture(texture, imFrame.data);
            }

            // Refill audio stream if required
            while (IsAudioStreamProcessed(stream))
            {
                // Decode audio sample
                sample = plm_decode_audio(plm);
                
                // Copy finished frame to audio stream
                UpdateAudioStream(stream, sample->interleaved, PLM_AUDIO_SAMPLES_PER_FRAME*2);
            }
        }
        //-----------------------------------------------------

        // Draw
        //-----------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawTexture(texture, GetScreenWidth()/2 - texture.width/2, GetScreenHeight()/2 - texture.height/2, WHITE);

        EndDrawing();
        //-----------------------------------------------------
    }

    // De-Initialization
    //---------------------------------------------------------
    UnloadImage(imFrame);
    UnloadTexture(texture);
    
    CloseAudioStream(stream);
    CloseAudioDevice();

    plm_destroy(plm);
    
    CloseWindow();        // Close window and OpenGL context
    //----------------------------------------------------------

    return 0;
}