#include "images.h"

// Dice images
LV_IMG_DECLARE(d4);
LV_IMG_DECLARE(d6);
LV_IMG_DECLARE(d8);
LV_IMG_DECLARE(d10);
LV_IMG_DECLARE(d12);
LV_IMG_DECLARE(d20);
LV_IMG_DECLARE(dpercent);
LV_IMG_DECLARE(nodie);
LV_IMG_DECLARE(dicemo);
LV_IMG_DECLARE(dicemo_mad);
LV_IMG_DECLARE(clearfav);
LV_IMG_DECLARE(clearmemory);
LV_IMG_DECLARE(mute);
LV_IMG_DECLARE(volume);
LV_IMG_DECLARE(wifi);

// Background frames (animated)
LV_IMG_DECLARE(bg00);
LV_IMG_DECLARE(bg01);
LV_IMG_DECLARE(bg02);
LV_IMG_DECLARE(bg03);
LV_IMG_DECLARE(bg04);
LV_IMG_DECLARE(bg05);
LV_IMG_DECLARE(bg06);
LV_IMG_DECLARE(bg07);
LV_IMG_DECLARE(bg08);
LV_IMG_DECLARE(bg09);
LV_IMG_DECLARE(bg10);
LV_IMG_DECLARE(bg11);
LV_IMG_DECLARE(bg12);
LV_IMG_DECLARE(bg13);
LV_IMG_DECLARE(bg14);
LV_IMG_DECLARE(bg15);

// Firey background frames (animated)
LV_IMG_DECLARE(fire00);
LV_IMG_DECLARE(fire01);
LV_IMG_DECLARE(fire02);
LV_IMG_DECLARE(fire03);
LV_IMG_DECLARE(fire04);
LV_IMG_DECLARE(fire05);
LV_IMG_DECLARE(fire06);
LV_IMG_DECLARE(fire07);
LV_IMG_DECLARE(fire08);
LV_IMG_DECLARE(fire09);
LV_IMG_DECLARE(fire10);
LV_IMG_DECLARE(fire11);

const lv_img_dsc_t *bg_frames[16] = {
    &bg00, &bg01, &bg02, &bg03,
    &bg04, &bg05, &bg06, &bg07,
    &bg08, &bg09, &bg10, &bg11,
    &bg12, &bg13, &bg14, &bg15
};

const lv_img_dsc_t *fire_frames[12] = {
    &fire00, &fire01, &fire02, &fire03,
    &fire04, &fire05, &fire06, &fire07,
    &fire08, &fire09, &fire10, &fire11
};
const int NUM_FIRE_FRAMES = sizeof(fire_frames) / sizeof(fire_frames[0]);