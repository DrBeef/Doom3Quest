// Copyright 2011 Google Inc. All Rights Reserved.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
// Alpha-plane compression.
//
// Author: Skal (pascal.massimino@gmail.com)

#include <assert.h>
#include <stdlib.h>

#include "./vp8enci.h"
#include "../utils/filters.h"
#include "../utils/quant_levels.h"
#include "../webp/format_constants.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Encodes the given alpha data via specified compression method 'method'.
// The pre-processing (quantization) is performed if 'quality' is less than 100.
// For such cases, the encoding is lossy. The valid range is [0, 100] for
// 'quality' and [0, 1] for 'method':
//   'method = 0' - No compression;
//   'method = 1' - Use lossless coder on the alpha plane only
// 'filter' values [0, 4] correspond to prediction modes none, horizontal,
// vertical & gradient filters. The prediction mode 4 will try all the
// prediction modes 0 to 3 and pick the best one.
// 'effort_level': specifies how much effort must be spent to try and reduce
//  the compressed output size. In range 0 (quick) to 6 (slow).
//
// 'output' corresponds to the buffer containing compressed alpha data.
//          This buffer is allocated by this method and caller should call
//          free(*output) when done.
// 'output_size' corresponds to size of this compressed alpha buffer.
//
// Returns 1 on successfully encoding the alpha and
//         0 if either:
//           invalid quality or method, or
//           memory allocation for the compressed data fails.

#include "../enc/vp8li.h"

static int EncodeLossless(const uint8_t* const data, int width, int height,
                          int effort_level,  // in [0..6] range
                          VP8BitWriter* const bw,
                          WebPAuxStats* const stats) {
  int ok = 0;
  WebPConfig config;
  WebPPicture picture;
  VP8LBitWriter tmp_bw;

  WebPPictureInit(&picture);
  picture.width = width;
  picture.height = height;
  picture.use_argb = 1;
  picture.stats = stats;
  if (!WebPPictureAlloc(&picture)) return 0;

  // Transfer the alpha values to the green channel.
  {
    int i, j;
    uint32_t* dst = picture.argb;
    const uint8_t* src = data;
    for (j = 0; j < picture.height; ++j) {
      for (i = 0; i < picture.width; ++i) {
        dst[i] = (src[i] << 8) | 0xff000000u;
      }
      src += width;
      dst += picture.argb_stride;
    }
  }

  WebPConfigInit(&config);
  config.lossless = 1;
  config.method = effort_level;  // impact is very small
  // Set a moderate default quality setting for alpha.
  config.quality = 10.f * effort_level;
  assert(config.quality >= 0 && config.quality <= 100.f);

  ok = VP8LBitWriterInit(&tmp_bw, (width * height) >> 3);
  ok = ok && (VP8LEncodeStream(&config, &picture, &tmp_bw) == VP8_ENC_OK);
  WebPPictureFree(&picture);
  if (ok) {
    const uint8_t* const buffer = VP8LBitWriterFinish(&tmp_bw);
    const size_t buffer_size = VP8LBitWriterNumBytes(&tmp_bw);
    VP8BitWriterAppend(bw, buffer, buffer_size);
  }
  VP8LBitWriterDestroy(&tmp_bw);
  return ok && !bw->error_;
}

// -----------------------------------------------------------------------------

// This function always returns an initialized 'bw' object, even upon error.
static int EncodeAlphaInternal(const uint8_t* const data, int width, int height,
                               int method, int filter, int reduce_levels,
                               int effort_level,  // in [0..6] range
                               uint8_t* const tmp_alpha,
                               VP8BitWriter* const bw,
                               WebPAuxStats* const stats) {
  int ok = 0;
  const uint8_t* alpha_src;
  WebPFilterFunc filter_func;
  uint8_t header;
  size_t expected_size;
  const size_t data_size = width * height;

  assert((uint64_t)data_size == (uint64_t)width * height);  // as per spec
  assert(filter >= 0 && filter < WEBP_FILTER_LAST);
  assert(method >= ALPHA_NO_COMPRESSION);
  assert(method <= ALPHA_LOSSLESS_COMPRESSION);
  assert(sizeof(header) == ALPHA_HEADER_LEN);
  // TODO(skal): have a common function and #define's to validate alpha params.

  expected_size =
      (method == ALPHA_NO_COMPRESSION) ? (ALPHA_HEADER_LEN + data_size)
                                       : (data_size >> 5);
  header = method | (filter << 2);
  if (reduce_levels) header |= ALPHA_PREPROCESSED_LEVELS << 4;

  VP8BitWriterInit(bw, expected_size);
  VP8BitWriterAppend(bw, &header, ALPHA_HEADER_LEN);

  filter_func = WebPFilters[filter];
  if (filter_func != NULL) {
    filter_func(data, width, height, width, tmp_alpha);
    alpha_src = tmp_alpha;
  }  else {
    alpha_src = data;
  }

  if (method == ALPHA_NO_COMPRESSION) {
    ok = VP8BitWriterAppend(bw, alpha_src, width * height);
    ok = ok && !bw->error_;
  } else {
    ok = EncodeLossless(alpha_src, width, height, effort_level, bw, stats);
    VP8BitWriterFinish(bw);
  }
  return ok;
}

// -----------------------------------------------------------------------------

// TODO(skal): move to dsp/ ?
static void CopyPlane(const uint8_t* src, int src_stride,
                      uint8_t* dst, int dst_stride, int width, int height) {
  while (height-- > 0) {
    memcpy(dst, src, width);
    src += src_stride;
    dst += dst_stride;
  }
}

static int GetNumColors(const uint8_t* data, int width, int height,
                        int stride) {
  int j;
  int colors = 0;
  uint8_t color[256] = { 0 };

  for (j = 0; j < height; ++j) {
    int i;
    const uint8_t* const p = data + j * stride;
    for (i = 0; i < width; ++i) {
      color[p[i]] = 1;
    }
  }
  for (j = 0; j < 256; ++j) {
    if (color[j] > 0) ++colors;
  }
  return colors;
}

// Given the input 'filter' option, return an OR'd bit-set of filters to try.
static uint32_t GetFilterMap(const uint8_t* alpha, int width, int height,
                             int filter, int effort_level) {
  uint32_t bit_map = 0U;
  if (filter == WEBP_FILTER_FAST) {
    // Quick estimate of the best candidate.
    int try_filter_none = (effort_level > 3);
    const int kMinColorsForFilterNone = 16;
    const int kMaxColorsForFilterNone = 192;
    const int num_colors = GetNumColors(alpha, width, height, width);
    // For low number of colors, NONE yeilds better compression.
    filter = (num_colors <= kMinColorsForFilterNone) ? WEBP_FILTER_NONE :
             EstimateBestFilter(alpha, width, height, width);
    bit_map |= 1 << filter;
    // For large number of colors, try FILTER_NONE in addition to the best
    // filter as well.
    if (try_filter_none || num_colors > kMaxColorsForFilterNone) {
      bit_map |= 1 << WEBP_FILTER_NONE;
    }
  } else if (filter == WEBP_FILTER_NONE) {
    bit_map = 1 << WEBP_FILTER_NONE;
  } else {  // WEBP_FILTER_BEST
    bit_map = (1 << WEBP_FILTER_LAST) - 1;  // try all.
  }
  return bit_map;
}

// Small struct to hold the result of a filter mode compression attempt.
typedef struct {
  size_t score;
  VP8BitWriter bw;
  WebPAuxStats stats;
} FilterTrial;

static void InitFilterTrial(FilterTrial* const score) {
  score->score = (size_t)~0U;
  VP8BitWriterInit(&score->bw, 0);
}

static int ApplyFilters(const uint8_t* alpha, int width, int height,
                        uint64_t data_size, int method, int filter,
                        int reduce_levels, int effort_level,
                        uint8_t** const output, size_t* const output_size,
                        WebPAuxStats* const stats) {
  int ok = 1;
  uint8_t* filtered_alpha = NULL;
  uint32_t try_map = GetFilterMap(alpha, width, height, filter, effort_level);

  filtered_alpha = (uint8_t*)malloc(data_size);
  ok = (filtered_alpha != NULL);

  if (ok) {
    FilterTrial best;
    InitFilterTrial(&best);

    for (filter = WEBP_FILTER_NONE; try_map; ++filter, try_map >>= 1) {
      if (try_map & 1) {
        FilterTrial trial;
        ok = EncodeAlphaInternal(alpha, width, height, method, filter,
                                 reduce_levels, effort_level, filtered_alpha,
                                 &trial.bw, stats);
        if (ok) {
          trial.score = VP8BitWriterSize(&trial.bw);
          if (trial.score < best.score) {
            VP8BitWriterWipeOut(&best.bw);
            best = trial;
            if (stats != NULL) best.stats = *stats;
          } else {
            VP8BitWriterWipeOut(&trial.bw);
          }
        } else {
          VP8BitWriterWipeOut(&trial.bw);
          VP8BitWriterWipeOut(&best.bw);
          break;
        }
      }
    }

    if (ok) {
      if (stats != NULL) *stats = best.stats;
      *output_size = VP8BitWriterSize(&best.bw);
      *output = VP8BitWriterBuf(&best.bw);
    }
    free(filtered_alpha);
  }
  return ok;
}

static int EncodeAlpha(VP8Encoder* const enc,
                       int quality, int method, int filter,
                       int effort_level,
                       uint8_t** const output, size_t* const output_size) {
  const WebPPicture* const pic = enc->pic_;
  const int width = pic->width;
  const int height = pic->height;

  uint8_t* quant_alpha = NULL;
  const size_t data_size = width * height;
  uint64_t sse = 0;
  int ok = 1;
  const int reduce_levels = (quality < 100);

  // quick sanity checks
  assert((uint64_t)data_size == (uint64_t)width * height);  // as per spec
  assert(enc != NULL && pic != NULL && pic->a != NULL);
  assert(output != NULL && output_size != NULL);
  assert(width > 0 && height > 0);
  assert(pic->a_stride >= width);
  assert(filter >= WEBP_FILTER_NONE && filter <= WEBP_FILTER_FAST);

  if (quality < 0 || quality > 100) {
    return 0;
  }

  if (method < ALPHA_NO_COMPRESSION || method > ALPHA_LOSSLESS_COMPRESSION) {
    return 0;
  }

  quant_alpha = (uint8_t*)malloc(data_size);
  if (quant_alpha == NULL) {
    return 0;
  }

  // Extract alpha data (width x height) from raw_data (stride x height).
  CopyPlane(pic->a, pic->a_stride, quant_alpha, width, width, height);

  if (reduce_levels) {  // No Quantization required for 'quality = 100'.
    // 16 alpha levels gives quite a low MSE w.r.t original alpha plane hence
    // mapped to moderate quality 70. Hence Quality:[0, 70] -> Levels:[2, 16]
    // and Quality:]70, 100] -> Levels:]16, 256].
    const int alpha_levels = (quality <= 70) ? (2 + quality / 5)
                                             : (16 + (quality - 70) * 8);
    ok = QuantizeLevels(quant_alpha, width, height, alpha_levels, &sse);
  }

  if (ok) {
    ok = ApplyFilters(quant_alpha, width, height, data_size, method, filter,
                      reduce_levels, effort_level, output, output_size,
                      pic->stats);
    if (pic->stats != NULL) {  // need stats?
      pic->stats->coded_size += (int)(*output_size);
      enc->sse_[3] = sse;
    }
  }

  free(quant_alpha);
  return ok;
}

//------------------------------------------------------------------------------
// Main calls

static int CompressAlphaJob(VP8Encoder* const enc, void* dummy) {
  const WebPConfig* config = enc->config_;
  uint8_t* alpha_data = NULL;
  size_t alpha_size = 0;
  const int effort_level = config->method;  // maps to [0..6]
  const WEBP_FILTER_TYPE filter =
      (config->alpha_filtering == 0) ? WEBP_FILTER_NONE :
      (config->alpha_filtering == 1) ? WEBP_FILTER_FAST :
                                       WEBP_FILTER_BEST;
  if (!EncodeAlpha(enc, config->alpha_quality, config->alpha_compression,
                   filter, effort_level, &alpha_data, &alpha_size)) {
    return 0;
  }
  if (alpha_size != (uint32_t)alpha_size) {  // Sanity check.
    free(alpha_data);
    return 0;
  }
  enc->alpha_data_size_ = (uint32_t)alpha_size;
  enc->alpha_data_ = alpha_data;
  (void)dummy;
  return 1;
}

void VP8EncInitAlpha(VP8Encoder* const enc) {
  enc->has_alpha_ = WebPPictureHasTransparency(enc->pic_);
  enc->alpha_data_ = NULL;
  enc->alpha_data_size_ = 0;
  if (enc->thread_level_ > 0) {
    WebPWorker* const worker = &enc->alpha_worker_;
    WebPWorkerInit(worker);
    worker->data1 = enc;
    worker->data2 = NULL;
    worker->hook = (WebPWorkerHook)CompressAlphaJob;
  }
}

int VP8EncStartAlpha(VP8Encoder* const enc) {
  if (enc->has_alpha_) {
    if (enc->thread_level_ > 0) {
      WebPWorker* const worker = &enc->alpha_worker_;
      if (!WebPWorkerReset(worker)) {    // Makes sure worker is good to go.
        return 0;
      }
      WebPWorkerLaunch(worker);
      return 1;
    } else {
      return CompressAlphaJob(enc, NULL);   // just do the job right away
    }
  }
  return 1;
}

int VP8EncFinishAlpha(VP8Encoder* const enc) {
  if (enc->has_alpha_) {
    if (enc->thread_level_ > 0) {
      WebPWorker* const worker = &enc->alpha_worker_;
      if (!WebPWorkerSync(worker)) return 0;  // error
    }
  }
  return WebPReportProgress(enc->pic_, enc->percent_ + 20, &enc->percent_);
}

int VP8EncDeleteAlpha(VP8Encoder* const enc) {
  int ok = 1;
  if (enc->thread_level_ > 0) {
    WebPWorker* const worker = &enc->alpha_worker_;
    ok = WebPWorkerSync(worker);  // finish anything left in flight
    WebPWorkerEnd(worker);  // still need to end the worker, even if !ok
  }
  free(enc->alpha_data_);
  enc->alpha_data_ = NULL;
  enc->alpha_data_size_ = 0;
  enc->has_alpha_ = 0;
  return ok;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    // extern "C"
#endif
