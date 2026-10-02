// block_quant.hpp
// Umbrella header — include THIS one from your PC and ESP32 code, not
// the individual bq_*.hpp files directly. Pulls in common + all quant
// schemes so a config swap is always "change one `using Scheme = ...`
// line", never "go add an #include somewhere".
//
// To add a new quant scheme file later: write bq_quant_<name>.hpp
// following bq_quant_flat.hpp's shape (own Wire{}, own encode/decode),
// then add one #include line below. That's the entire integration cost.

#pragma once

#include "quant/bq_common.hpp"
#include "quant/bq_quant_flat.hpp"
#include "quant/bq_quant_hierarchical.hpp"
// #include "bq_quant_<your_next_scheme>.hpp"

/* ---------------------------------------------------------------------
 * PC-side usage (in a PC-only .cpp):
 *
 *   #include "block_quant.hpp"
 *   using Cfg    = bq::Cfg4x4_RGB565;
 *   using Scheme = bq::FlatMinMaxQuant<Cfg, 2>;   // <- swap this line
 *                                                  //    to test a
 *                                                  //    different scheme
 *   int grid_w = frame_w / Cfg::block_w, grid_h = frame_h / Cfg::block_h;
 *   std::vector<Scheme::Wire> blocks(grid_w * grid_h);
 *
 *   #pragma omp parallel for collapse(2)
 *   for (int by = 0; by < grid_h; ++by)
 *     for (int bx = 0; bx < grid_w; ++bx) {
 *       int idx = by * grid_w + bx;
 *       blocks[idx].block_x = bx; blocks[idx].block_y = by;
 *       Scheme::encode(src_buf, src_stride, bx*Cfg::block_w, by*Cfg::block_h,
 *                      blocks[idx]);
 *     }
 *   // send only the DIRTY subset of `blocks` over UDP (delta-encoding
 *   // step lives outside this header entirely)
 *
 * ESP32-side usage:
 *
 *   #include "block_quant.hpp"
 *   Scheme::Wire incoming;  // fill from UDP recv buffer
 *   Scheme::decode(incoming, framebuffer, panel_width_px,
 *                  incoming.block_x * Cfg::block_w,
 *                  incoming.block_y * Cfg::block_h);
 *
 * Both sides MUST use the identical Cfg + Scheme instantiation, or the
 * Wire struct's byte layout won't match across the wire. Put the
 * `using Cfg = ...; using Scheme = ...;` pair in ONE place both PC and
 * ESP32 code includes (this file's a good spot, or a small
 * bq_active_config.hpp you swap per test run) rather than duplicating
 * it on each side, since a mismatch there is a silent wire-format bug,
 * not a compile error.
 * --------------------------------------------------------------------- */
