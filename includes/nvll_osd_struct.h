/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. The names of its contributors may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * <b>NVIDIA Multimedia Utilities: On-Screen Display Manager</b>
 *
 * This file defines the NvOSD library to be used to draw rectangles and text
 * over the frame.
 */

/**
 * @defgroup ee_nvosd_group On-Screen Display Manager
 * Defines the NvOSD library to be used to draw rectangles and text
 * over the frame.
 * @ingroup NvDsOsdApi
 * @{
 */

#ifndef __NVLL_OSD_STRUCT_DEFS__
#define __NVLL_OSD_STRUCT_DEFS__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Defines modes used to overlay boxes and text.
 */
typedef enum {
    MODE_CPU, /**< Specifies using the CPU for OSD processing.
                Works with RGBA data only */
    MODE_GPU, /**< Specifies using the GPU for OSD processing.
                Currently not implemented. */
    MODE_HW   /**< Specifies the NVIDIA hardware engine
               for rectangle drawing and masking.
                   This mode works with both YUV and RGB data.
                   It does not consider alpha parameter.
                   Not applicable to drawing text. */
} NvOSD_Mode;

/**
 * Specifies arrow head positions.
 */
typedef enum
{
  /** Specifies an arrow head only at start = 0. */
  START_HEAD,
  /** Specifies an arrow head only at end = 1. */
  END_HEAD,
  /** Specifies arrow heads at both start and end = 2. */
  BOTH_HEAD
} NvOSD_Arrow_Head_Direction;

/**
 * Holds the color parameters of the box or text to be overlayed.
 */
typedef struct _NvOSD_ColorParams {
  double red;     /**< Holds the red component of color.
                   Value must be in the range 0.0-1.0. */

  double green;   /**< Holds the green component of color.
                   Value must be in the range 0.0-1.0.*/

  double blue;    /**< Holds the blue component of color.
                   Value must be in the range 0.0-1.0.*/

  double alpha;   /**< Holds the alpha component of color.
                   Value must be in the range 0.0-1.0.*/
} NvOSD_ColorParams;

/**
 * Holds the font parameters of the text to be overlayed.
 */
typedef struct _NvOSD_FontParams {
  char * font_name;             /**< Holds a pointer to the string containing
                                 the font name. To display a list of
                                 supported fonts, run the fc-list command. */

//  char font_name[64];         /**< Holds a pointer to a string containing
//                               the font name. */

  unsigned int font_size;       /**< Holds the size of the font. */

  NvOSD_ColorParams font_color; /**< Holds the font color. */
} NvOSD_FontParams;


/**
 * Holds parameters of text to be overlayed.
 */
typedef struct _NvOSD_TextParams {
  char * display_text;          /**< Holds the text to be overlayed. */

  unsigned int x_offset;        /**< Holds the text's horizontal offset from
                                 the top left pixel of the frame. */
  unsigned int y_offset;        /**< Holds the text's vertical offset from the
                                 top left pixel of the frame. */
  NvOSD_FontParams font_params; /**< Holds the font parameters of the text
                                 to be overlaid. */
  int set_bg_clr;               /**< Holds a Boolean; true if the text has a
                                 background color. */

  NvOSD_ColorParams text_bg_clr;/**< Holds the text's background color, if
                                 specified. */

} NvOSD_TextParams;

typedef struct _NvOSD_Color_info {
    int id;
    NvOSD_ColorParams color;
}NvOSD_Color_info;

/**
 * Holds the box parameters of the box to be overlayed.
 */
typedef struct _NvOSD_RectParams {
  float left;            /**< Holds the box's left coordinate
                                     in pixels. */

  float top;             /**< Holds the box's top coordinate
                                     in pixels. */

  float width;           /**< Holds the box's width in pixels. */

  float height;          /**< Holds the box's height in pixels. */

  unsigned int border_width;    /**< Holds the box's border width in pixels. */

  NvOSD_ColorParams border_color;
                                /**< Holds the box's border color. */

  unsigned int has_bg_color;    /**< Holds a Boolean; true if the box has a
                                 background color. */

  unsigned int reserved;        /**< Holds a field reserved for future use. */

  NvOSD_ColorParams bg_color;   /**< Holds the box's background color. */

  int has_color_info;
  int color_id;
} NvOSD_RectParams;

/**
 * Holds the box parameters of a line to be overlayed.
 */
typedef struct _NvOSD_LineParams {
  unsigned int x1;              /**< Holds the box's left coordinate
                                 in pixels. */

  unsigned int y1;              /**< Holds the box's top coordinate
                                 in pixels. */

  unsigned int x2;              /**< Holds the box's width in pixels. */

  unsigned int y2;              /**< Holds the box's height in pixels. */

  unsigned int line_width;      /**< Holds the box's border width in pixels. */

  NvOSD_ColorParams line_color; /**< Holds the box's border color. */
} NvOSD_LineParams;

/**
 * Holds arrow parameters to be overlaid.
 */
typedef struct _NvOSD_ArrowParams {
  unsigned int x1;   /**< Holds the start horizontal coordinate in pixels. */

  unsigned int y1;    /**< Holds the start vertical coordinate in pixels. */

  unsigned int x2;  /**< Holds the end horizontal coordinate in pixels. */

  unsigned int y2; /**< Holds the end vertical coordinate in pixels. */

  unsigned int arrow_width; /**< Holds the arrow shaft width in pixels. */

  NvOSD_Arrow_Head_Direction arrow_head;
                            /**< Holds the arrowhead position. */

  NvOSD_ColorParams arrow_color;
                            /**< Holds color parameters of the arrow box. */

  unsigned int reserved;    /**< Reserved for future use; currently
                               for internal use only. */
} NvOSD_ArrowParams;

/**
 * Holds circle parameters to be overlayed.
 */
typedef struct _NvOSD_CircleParams {
  unsigned int xc;   /**< Holds the start horizontal coordinate in pixels. */

  unsigned int yc;    /**< Holds the start vertical coordinate in pixels. */

  unsigned int radius;  /**< Holds the radius of circle in pixels. */

  NvOSD_ColorParams circle_color;
                        /**< Holds the color parameters of the arrow box. */

  unsigned int has_bg_color;   /*< Holds a Boolean value indicating whether
                                   the circle has a background color. */

  NvOSD_ColorParams bg_color;  /*< Holds the circle's background color. */

  unsigned int reserved; /**< Reserved for future use; currently
                            for internal use only. */

} NvOSD_CircleParams;

#ifdef __cplusplus
}
#endif
/** @} */
#endif
