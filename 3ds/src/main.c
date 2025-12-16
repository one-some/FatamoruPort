#include <citro2d.h>
#include <3ds.h>

int main(int argc, char* argv[]) {
    // 1. Initialization
    romfsInit();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    // C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    // C2D_Prepare();

    consoleInit(GFX_BOTTOM, NULL);
    printf("FataMoru\n"); // Green text
	C3D_Tex tex;
    bool vram_success = C3D_TexInitVRAM(&tex, 256, 512, GPU_RGBA8);
	if (!vram_success) printf("UH OHHHH\n");
    vram_success = C3D_TexInitVRAM(&tex, 256, 512, GPU_RGBA8);
	if (!vram_success) printf("uh oh 2\n");

    // 2. Setup
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/bgimage/3章_カーザノストラ.t3x");


    // 3. Create Sprite
    C2D_Sprite player;
    if (spriteSheet) {
        // Automatically sets width/height from the texture
        C2D_SpriteFromSheet(&player, spriteSheet, 0);
        C2D_SpriteSetCenter(&player, 0.5f, 0.5f);
        C2D_SpriteSetPos(&player, 200.0f, 120.0f);
    } else {
        printf("What the hell\n");
    }

    // 4. Main Loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;

        // Simple movement logic
        if (kDown & KEY_UP)    player.params.pos.y -= 10.0f;
        if (kDown & KEY_DOWN)  player.params.pos.y += 10.0f;
        if (kDown & KEY_LEFT)  player.params.pos.x -= 10.0f;
        if (kDown & KEY_RIGHT) player.params.pos.x += 10.0f;

        // Render
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32f(0.2f, 0.2f, 0.2f, 1.0f));
        C2D_SceneBegin(top);

        if (spriteSheet) C2D_DrawSprite(&player);

        C3D_FrameEnd(0);
    }

    // 5. Cleanup
    C2D_SpriteSheetFree(spriteSheet);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
    return 0;
}
