#ifndef IMAGES_H
#define IMAGES_H

#include <lvgl.h>

// Dice images
extern const lv_img_dsc_t d4;
extern const lv_img_dsc_t d6;
extern const lv_img_dsc_t d8;
extern const lv_img_dsc_t d10;
extern const lv_img_dsc_t d12;
extern const lv_img_dsc_t d20;
extern const lv_img_dsc_t dpercent;
extern const lv_img_dsc_t nodie; // Empty state
extern const lv_img_dsc_t dicemo;
extern const lv_img_dsc_t dicemo_mad;
extern const lv_img_dsc_t clearmemory;
extern const lv_img_dsc_t clearfav;
extern const lv_img_dsc_t mute;
extern const lv_img_dsc_t volume;
extern const lv_img_dsc_t wifi;

// Background frames
extern const lv_img_dsc_t *bg_frames[16];
extern const lv_img_dsc_t *fire_frames[12];
extern const int NUM_FIRE_FRAMES;

#endif // IMAGES_H