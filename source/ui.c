#include <stdlib.h>
#include <3ds.h>

#include "pp2d/pp2d.h"
#include "filestuff.h"
#include "ui.h"

#define WHITE   RGBA8(255, 255, 255, 255)
#define GREYBG  RGBA8(255, 120, 120, 120)
#define LGREYBG RGBA8(255, 200, 200, 200)
#define LGREYFG RGBA8(200, 200, 200, 255)
#define GREYFG  RGBA8(120, 120, 120, 255)
#define GREY2FG RGBA8(80,   80,  80, 255)
#define DGREY   RGBA8( 50,  50,  50, 255)

#define MAXSIZE 255
#define NUMBTNS 8

// ------------------------------------------------------
// UI Mod Select Variables
// ------------------------------------------------------

// globals, probably important
UIButton** buttonList;
    // 0 -> Launch Game
    // 1 -> Save Config
    // 2 -> Load Config
    // 3 -> Go Back

// required for listing files in mod select
char** modListing;
u16 modCount;
u8* modSelected;

s16 entryIndex = 0;  // position of index in array
u16 indexPos = 0;    // position of screen in array
u8 cursorPos = 0;    // is the cursor in the file browser or the button list?
s16 buttonIndex = 0;

// END
// ------------------------------------------------------


// for debugging
void stall() {
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown)
            break;
        
        gfxFlushBuffers();
        gfxSwapBuffers();
    }
}

void loadFromFile(char* desc) {
    char* path = malloc(MAXSIZE * sizeof(char*));
    
    snprintf(path, MAXSIZE, "/3ds/data/Haxelektor/test/%s/description.txt", modListing[entryIndex]);
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        snprintf(desc, MAXSIZE, "No description provided.");
        return;
    }
    
    fgets(desc, MAXSIZE, fp);
    fclose(fp);
    
    free(path);
}
    
void createButton(UIButton* but, int x, int y, int width, int height, int textIndex) {
    but->x = x;
    but->y = y;
    but->width = width;
    but->height = height;
    
    but->text = malloc(MAXSIZE * sizeof(char*));
    switch (textIndex) {
        case 0:
            snprintf(but->text, MAXSIZE, "Launch Game");
            break;
        case 1:
            snprintf(but->text, MAXSIZE, "Apply Patches");
            break;
        case 2:
            snprintf(but->text, MAXSIZE, "Remove Patches");
            break;
        case 3:
            snprintf(but->text, MAXSIZE, "Save Config");
            break;
        case 4:
            snprintf(but->text, MAXSIZE, "Load Config");
            break;
        case 5:
            snprintf(but->text, MAXSIZE, "Go Back");
            break;
        case 6:
            snprintf(but->text, MAXSIZE, "<<");
            break;
        case 7:
            snprintf(but->text, MAXSIZE, ">>");
            break;
        default:
            snprintf(but->text, MAXSIZE, "whoops, that wasn't supposed to happen");
            break;
    }
}

void drawButton(UIButton* but, bool selected) {
    u32 color;
    
    int textWidth = pp2d_get_text_width(but->text, 0.4f, 0.4f);
    if (selected)
        color = LGREYFG;
    else
        color = GREYFG;
    
    pp2d_draw_rectangle(but->x, but->y, but->width, but->height, color);
    pp2d_draw_text(but->x + (but->width / 2) - (textWidth / 2), but->y + (but->height / 3), 0.4f, 0.4f, WHITE, but->text);
}

bool isButtonTouched(UIButton* b, touchPosition* t) {
    return (b->x <= t->px && t->px <= b->x + b->width) && \
           (b->y <= t->py && t->py <= b->y + b->height);
}

s16 getCurrentButtonTouched(touchPosition* t) {
    for (int i = 0; i < NUMBTNS; i++) {
        if (isButtonTouched(buttonList[i], t)) 
            return i;
    }
        
    return -1;
}

void createModEntry(GameMod* mod, char* name, char* description, int imgID) {
    snprintf(mod->name, MAXSIZE, name);
    snprintf(mod->description, MAXSIZE, description);
    
    mod->imgID = imgID;
    mod->isSelected = 0;
}

void uiInit() {
    // create them buttons
    buttonList = malloc(NUMBTNS * sizeof(*buttonList));
    for (int i = 0; i < NUMBTNS; i++) {
        buttonList[i] = malloc(sizeof(UIButton*));
        createButton(buttonList[i], 220, 10 + (i * 30), 95, 27, i);
    }
    
    // Nothing like HARDCODING SHIT IN WHEN IT WON'T WORK FOR SOME STUPID-ASS FUCKING REASON OTHERWISE
    buttonList[0]->height = 25;
    buttonList[1]->height = 25;
    buttonList[2]->height = 25;
    buttonList[3]->height = 25;
    buttonList[4]->height = 25;
    buttonList[5]->height = 25;
    
    buttonList[6]->width = 45;
    buttonList[6]->height = 25;
    buttonList[7]->width = 45;
    buttonList[7]->height = 25;
    
    buttonList[7]->x = 270;
    buttonList[7]->y = buttonList[6]->y;
    
    // create and fill mod list
    modListing = listAllFiles("/3ds/data/Haxelektor/test/", &modCount, 1);
    modSelected = malloc(modCount * sizeof(u8*));
    
    char* path = malloc(MAXSIZE * sizeof(char*));
    for (int i = 0; i < modCount; i++) {
        modSelected[i] = 0;
        
        memset(path, 0, MAXSIZE * sizeof(char*));
        snprintf(path, MAXSIZE, "3ds/data/Haxelektor/test/%s/image.png", modListing[i]);
        pp2d_load_texture_png(i, path);
    }
    
    free(path);
}

LOOP_RETURN uiModSelectLoop() {
    pp2d_set_screen_color(GFX_TOP, LGREYBG);
    pp2d_set_screen_color(GFX_BOTTOM, LGREYBG);
    
    // debug shit
    //consoleInit(GFX_TOP, NULL);
    
    char desc[255];
    loadFromFile(desc);
    
    //printf("%s\n", desc);
    //stall();

    while (aptMainLoop()) {
        int add = 0;
        
        // read input
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        touchPosition touch;
        hidTouchRead(&touch);
        
        // react according to input
        if (kDown & KEY_START)
            break;
        
        // button input
        if (cursorPos == 0) {
            switch (kDown) {
                case KEY_UP:
                    entryIndex--;
                    loadFromFile(desc);
                    break;
                case KEY_DOWN:
                    entryIndex++;
                    loadFromFile(desc);
                    add = 1;
                    break;
                case KEY_LEFT:
                    cursorPos = !cursorPos;
                    break;
                case KEY_RIGHT:
                    cursorPos = !cursorPos;
                    break;
                case KEY_A:
                    modSelected[entryIndex] = !modSelected[entryIndex];
                    break;
                default:
                    break;
            };
            
            if (entryIndex >= modCount) {
                entryIndex = 0;
                indexPos = 0;
                loadFromFile(desc);
            } else if (entryIndex < 0) {
                entryIndex = modCount - 1;
                indexPos = ((int)ceil((float)modCount / 13.0) - 1) * 13;
                loadFromFile(desc);
            }
        
            if (entryIndex < indexPos) {
                indexPos = indexPos - 13;
                loadFromFile(desc);
            } else if ((entryIndex % 13) == 0 && entryIndex != 0 && add == 1) {
                indexPos = indexPos + 13;
                loadFromFile(desc);
            }
        } else if (cursorPos == 1) {
            switch (kDown) {
                case KEY_UP:
                    if (buttonIndex == 7)
                        buttonIndex--;
                    buttonIndex--;
                    break;
                case KEY_DOWN:
                    if (buttonIndex == 6)
                        buttonIndex++;
                    buttonIndex++;
                    break;
                case KEY_LEFT:
                    if (buttonIndex == 7)
                        buttonIndex--;
                    else if (buttonIndex == 6)
                        buttonIndex++;
                    else
                        cursorPos = !cursorPos;
                    break;
                case KEY_RIGHT:
                    if (buttonIndex == 6)
                        buttonIndex++;
                    else if (buttonIndex == 7)
                        buttonIndex--;
                    else
                        cursorPos = !cursorPos;
                    break;
                default:
                    break;
            };
            
            if (buttonIndex >= NUMBTNS)
                buttonIndex = 0;
            else if (buttonIndex < 0)
                buttonIndex = NUMBTNS - 1;
        }
        
        // touch input
        s16 buttonTouched = getCurrentButtonTouched(&touch);
        if (buttonTouched != -1)
            printf("button touched: %d\n", buttonTouched);
        
        if (kDown & KEY_TOUCH) {
            switch (buttonTouched) {
                // mixed these up, lol
                case 7:
                    entryIndex = entryIndex + 13;
                    if ((entryIndex >= modCount) && indexPos != ((int)ceil((float)modCount / 13.0) - 1) * 13) {
                        entryIndex = modCount - 1;
                    }
                    indexPos = indexPos + 13;
                    loadFromFile(desc);
                    break;
                case 6:
                    entryIndex = entryIndex - 13;
                    indexPos = indexPos - 13;
                    loadFromFile(desc);
                    break;
                default:
                    break;
            };
            
            s16 prevEntryIndex = entryIndex;
            for (int i = 0; i < 13; i++) {
                if ((0 <= touch.px && touch.px <= 200) && \
                   (20 + ((i % 13) * 15) <= touch.py && touch.py <= 20 + ((i % 13) * 15) + 15)) {
                       entryIndex = indexPos + i;
                       if (entryIndex == prevEntryIndex)
                           modSelected[indexPos + i] = !modSelected[indexPos + i];
                        else
                            loadFromFile(desc);
                        
                        
                        if (cursorPos != 0)
                            cursorPos = !cursorPos;
                        break;
                   }
            }
        }        
        

        
        // display to screen
        pp2d_begin_draw(GFX_TOP);
            pp2d_draw_texture(entryIndex, 0, 0);
            pp2d_draw_rectangle(0, 220, 400, 20, GREYFG);
            pp2d_draw_text(0, 223, 0.5f, 0.5f, WHITE, desc);
        pp2d_end_draw(); 
        
        pp2d_begin_draw(GFX_BOTTOM);
            // draw mod entries
            int max = (indexPos + 13 < modCount) ? indexPos + 13 : modCount;
            
            for (int i = indexPos; i < max; i++) {
                if (i == entryIndex && cursorPos == 0)
                    pp2d_draw_rectangle(0, 20 + ((i % 13) * 15), 320, 15, GREYFG);
                pp2d_draw_text(0, 20 + ((i % 13) * 15), 0.5f, 0.5f, WHITE, modListing[i]);
                if (modSelected[i] != 0) 
                    pp2d_draw_text(150, 20 + ((i % 13) * 15), 0.5f, 0.5f, WHITE, "Selected");
            }
        
            // draw side and bottom bar
            pp2d_draw_rectangle(0, 0, 320, 20, GREYFG);
            pp2d_draw_rectangle(210, 0, 120, 240, GREY2FG);
            pp2d_draw_rectangle(0, 220, 320, 220, DGREY);
        
            // draw all the side buttons
            for (int i = 0; i < NUMBTNS; i++) {
                drawButton(buttonList[i], (i == buttonIndex && cursorPos == 1) || isButtonTouched(buttonList[i], &touch));
            }
        pp2d_end_draw();
    }
    
    return GO_BACK;
}

void uiExit() {
    free(modListing);  
    free(buttonList);
    free(modSelected);
}