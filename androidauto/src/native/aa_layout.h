#ifndef AA_LAYOUT_H
#define AA_LAYOUT_H
#ifndef AA_CLASSIC_LAYOUT
#define AA_CLASSIC_LAYOUT 0
#endif
#ifndef AA_HD
#define AA_HD 0
#endif
#ifndef AA_PACKED_720
#define AA_PACKED_720 0
#endif
#ifndef AA_PACKED_1080
#define AA_PACKED_1080 0
#endif
#if AA_PACKED_720 && (AA_HD || !AA_CLASSIC_LAYOUT)
#error AA_PACKED_720 requires the 720p classic layout
#endif
#if AA_PACKED_1080 && (!AA_HD || !AA_CLASSIC_LAYOUT || AA_PACKED_720)
#error AA_PACKED_1080 requires the 1080p classic layout only
#endif
#define AA_VIDEO_WIDTH (AA_HD ? 1920 : 1280)
#define AA_VIDEO_HEIGHT (AA_HD ? 1080 : 720)
#define AA_CROP_X ((AA_PACKED_720 || AA_PACKED_1080) ? 0 : (AA_HD ? 240 : 160))
#define AA_CROP_Y (AA_PACKED_1080 ? 180 : (AA_PACKED_720 ? 120 : (AA_HD ? 270 : 180)))
#define AA_CROP_WIDTH (AA_PACKED_1080 ? 1920 : (AA_PACKED_720 ? 1280 : (AA_HD ? 1440 : 960)))
#define AA_CROP_HEIGHT (AA_PACKED_1080 ? 720 : (AA_PACKED_720 ? 480 : (AA_HD ? 540 : 360)))
#define AA_DPI (AA_PACKED_1080 ? 192 : (AA_PACKED_720 ? 128 : (AA_HD ? 144 : 96)))
#define AA_REAL_DPI (AA_PACKED_1080 ? 170 : (AA_PACKED_720 ? 113 : (AA_HD ? 128 : 85)))
/* Packed profile scales the accepted 720p inset distances by 4/3, rounded to
 * nearest integer. Physical edges differ by less than half an output pixel. */
#define AA_INSET_TOP (AA_PACKED_1080 ? 102 : (AA_PACKED_720 ? 68 : (AA_HD ? 77 : 51)))
#define AA_INSET_BOTTOM (AA_PACKED_1080 ? 194 : (AA_PACKED_720 ? 129 : (AA_HD ? 146 : 97)))
#define AA_INSET_SIDE (AA_PACKED_1080 ? 680 : (AA_PACKED_720 ? 453 : (AA_HD ? 510 : 340)))
#endif
