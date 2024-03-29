/*
 * Copyright (c) 2014, Derek Buitenhuis
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SMALLFLY_METRIC_H
#define SMALLFLY_METRIC_H

float metric_smallfry (uint8_t *inbuf, uint8_t *outbuf, int width, int height);
float metric_cor (uint8_t *inbuf, uint8_t *outbuf, int width, int height);
float metric_corsharp (uint8_t *inbuf, uint8_t *outbuf, int width, int height, int radius);
float metric_sharpenbad (uint8_t *inbuf, uint8_t *outbuf, int width, int height, int radius);
float metric_nhw (uint8_t *inbuf, uint8_t *outbuf, int width, int height);
float cor_sigma (float cor);
char* libsmallfry_version (void);

#endif // SMALLFLY_METRIC_H //
