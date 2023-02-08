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

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "smallfry.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define LIBSMALLFRYVERSION "0.2.0"

/* PSNR(a,b) = 10*log10(MAX * MAX / E((a-b)*(a-b)))
 * MAX = 255 */
static float factor_psnr (uint8_t *orig, uint8_t *cmp, int orig_stride, int cmp_stride, int width, int height, uint8_t max)
{
    uint8_t *old, *new;
    float delta, suml, sum, ret;
    int i, j;

    old = orig;
    new = cmp;

    sum = 0.0f;
    for (i = 0; i < height; i++)
    {
        suml = 0.0f;
        for (j = 0; j < width; j++)
        {
            delta = (float)old[j] - (float)new[j];
            suml += delta * delta;
        }
        old += orig_stride;
        new += cmp_stride;
        sum += suml;
    }

    ret  = sum / (float) (width * height);
    ret  = 10.0f * log10(65025.0f / ret);

    if (max > 128)
        ret /= 50.0f;
    else
        ret /= (0.0016f * (float) (max * max)) - (0.38f * (float) max + 72.5f);

    return MAX(MIN(ret, 1.0f), 0.0f);
}

/* AAE(a,b) = K * E(a(GRID) - b(GRID))
 * GRID = {{0,7},8} */
static float factor_aae (uint8_t *orig, uint8_t *cmp, int orig_stride, int cmp_stride, int width, int height, uint8_t max)
{
    uint8_t *old, *new;
    float suml, sum, cnf, cfmax, cf, ret;
    int i, j, cnt;

    old = orig;
    new = cmp;

    cnf = 0.0f;
    sum = 0.0f;
    for (i = 0; i < height; i++)
    {
        cnt = 0;
        suml = 0.0f;
        for (j = 7; j < width - 2; j += 8)
        {
            int o0, n0, o1h, n1h, o1nh, n1nh, o2h, n2h, d0, d1h, d1nh, d2h;
            float calc;

            cnt++;

            o0 = (int)old[j];
            n0 = (int)new[j];
            d0 = abs(o0 - n0);
            o1h = (int)old[j + 1];
            n1h = (int)new[j + 1];
            d1h = abs(o1h - n1h);
            o1nh = (int)old[j - 1];
            n1nh = (int)new[j - 1];
            d1nh = abs(o1nh - n1nh);
            o2h = (int)old[j + 2];
            n2h = (int)new[j + 2];
            d2h = abs(o2h - n2h);
            calc  = abs(d0 - d1h);
            calc /= (0.0001 + abs(d1nh - d0) + abs(d1h - d2h)) * 0.5f;

            if (calc > 5.0f)
                suml += 1.0f;
            else if (calc > 2.0)
                suml += (calc - 2.0f) / (5.0f - 2.0f);
        }
        old += orig_stride;
        new += cmp_stride;
        cnf += (float)cnt;
        sum += suml;
    }

    old = orig + 7 * orig_stride;
    new = cmp  + 7 * cmp_stride;

    for (i = 7; i < height - 2; i += 8)
    {
        cnt = 0;
        suml = 0.0f;
        for (j = 0; j < width; j++)
        {
            int o0, n0, o1v, n1v, o1nv, n1nv, o2v, n2v, d0, d1v, d1nv, d2v;
            float calc;

            cnt++;

            o0 = (int)old[j];
            n0 = (int)new[j];
            d0 = abs(o0 - n0);
            o1v = (int)old[j + orig_stride];
            n1v = (int)new[j + orig_stride];
            d1v = abs(o1v - n1v);
            o1nv = (int)old[j - orig_stride];
            n1nv = (int)new[j - orig_stride];
            d1nv = abs(o1nv - n1nv);
            o2v = (int)old[j + orig_stride + orig_stride];
            n2v = (int)new[j + orig_stride + orig_stride];
            d2v = abs(o2v - n2v);
            calc  = abs(d0 - d1v);
            calc /= (0.0001f + abs(d1nv - d0) + abs(d1v - d2v)) * 0.5f;

            if (calc > 5.0f)
                suml += 1.0f;
            else if (calc > 2.0f)
                suml += (calc - 2.0f) / (5.0f - 2.0f);
        }
        old += 8 * orig_stride;
        new += 8 * cmp_stride;
        cnf += (float)cnt;
        sum += suml;
    }

    ret = 1.0f;
    if (cnf > 0.0f)
        ret -= (sum / cnf);

    if (max > 128)
        cfmax = 0.65f;
    else
        cfmax = 1.0f - 0.35f * (float)max / 128.0f;

    cf = MAX(cfmax, MIN(1.0f, 0.25f + (1000.0f * cnf) / sum));
    ret *= cf;

    return ret;
}

static uint8_t maxluma (uint8_t *buf, int stride, int width, int height)
{
    uint8_t *in = buf;
    uint8_t max = 0;
    int i, j;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
            max = MAX(in[j], max);

        in += stride;
    }

    return max;
}

/* SMALLFRY(a,b) = (K1 * PSNR(a,b) + K2 * AAE(a,b))
 * K1 = 37.1891885161239
 * K2 = 78.5328607296973 */
float metric_smallfry (uint8_t *inbuf, uint8_t *outbuf, int width, int height)
{
    float p, a, b;
    uint8_t max;

    max = maxluma(inbuf, width, width, height);

    p = factor_psnr(inbuf, outbuf, width, width, width, height, max);
    a = factor_aae(inbuf, outbuf, width, width, width, height, max);

    b = p * 37.1891885161239f + a * 78.5328607296973f;

    return b;
}

/* COR(a,b) = E(a * b) / sqrt(E(a * a) * E(b * b)) */
float metric_cor (uint8_t *inbuf, uint8_t *outbuf, int width, int height)
{
    uint8_t *old, *new;
    float im1, im2;
    float sum1, sum2, sum1l, sum2l;
    float sum12, sumq1, sumq2, q12, sum12l, sumq1l, sumq2l, sumq, cor;
    int i, j, k, n;

    old = inbuf;
    new = outbuf;
    n = width * height;

    k = 0;
    sum1 = 0.0f;
    sum2 = 0.0f;
    for (i = 0; i < height; i++)
    {
        sum1l = 0.0f;
        sum2l = 0.0f;
        for (j = 0; j < width; j++)
        {
            im1 = (float)old[k];
            im2 = (float)new[k];
            sum1l += im1;
            sum2l += im2;
            k++;
        }
        sum1 += sum1l;
        sum2 += sum2l;
    }
    sum1 /= (float)n;
    sum2 /= (float)n;

    k = 0;
    sum12 = 0.0f;
    sumq1 = 0.0f;
    sumq2 = 0.0f;
    for (i = 0; i < height; i++)
    {
        sum12l = 0.0f;
        sumq1l = 0.0f;
        sumq2l = 0.0f;
        for (j = 0; j < width; j++)
        {
            im1 = (float)old[k];
            im1 -= sum1;
            im2 = (float)new[k];
            im2 -= sum2;
            q12 = (im1 * im2);
            q12 = (q12 < 0.0f) ? -q12 : q12;
            sum12l += q12;
            sumq1l += (im1 * im1);
            sumq2l += (im2 * im2);
            k++;
        }
        sum12 += sum12l;
        sumq1 += sumq1l;
        sumq2 += sumq2l;
    }
    sumq = sqrt(sumq1 * sumq2);
    if (sumq > 0.0f)
    {
        cor = sum12 / sumq;
    } else {
        cor = (sumq1 == sumq2) ? 1.0f : 0.0f;
    }
    cor = (cor < 0.0f) ? -cor : cor;

    return cor;
}

/* CORSH(a,b) = E(GR(a,r) * GR(b,r)) / sqrt(E(GR(a,r) * GR(a,r)) * E(GR(b,r) * GR(b,r))) */
float metric_corsharp (uint8_t *inbuf, uint8_t *outbuf, int width, int height, int radius)
{
    uint8_t *old, *new;
    float im1, im2, imf1, imf2, ims1, ims2;
    float sum1, sum2, sum1l, sum2l;
    float sum12, sumq1, sumq2, q12, sum12l, sumq1l, sumq2l, sumq, cor;
    int i, j, i0, j0, i1, j1, i2, j2, k, ki0, ki, kj, n;

    old = inbuf;
    new = outbuf;
    n = width * height;
    if (radius < 0) {radius = -radius;}

    k = 0;
    sum1 = 0.0f;
    sum2 = 0.0f;
    for (i = 0; i < height; i++)
    {
        sum1l = 0.0f;
        sum2l = 0.0f;
        for (j = 0; j < width; j++)
        {
            im1 = (float)old[k];
            im2 = (float)new[k];
            sum1l += im1;
            sum2l += im2;
            k++;
        }
        sum1 += sum1l;
        sum2 += sum2l;
    }
    sum1 /= (float)n;
    sum2 /= (float)n;

    k = 0;
    sum12 = 0.0f;
    sumq1 = 0.0f;
    sumq2 = 0.0f;
    for (i = 0; i < height; i++)
    {
        sum12l = 0.0f;
        sumq1l = 0.0f;
        sumq2l = 0.0f;
        i0 = i - radius;
        if (i0 < 0) {i0 = 0;}
        i2 = i + radius + 1;
        if (i2 > height) {i2 = height;}
        ki0 = i0 * width;
        for (j = 0; j < width; j++)
        {
            j0 = j - radius;
            if (j0 < 0) {j0 = 0;}
            j2 = j + radius + 1;
            if (j2 > width) {j2 = width;}
            im1 = (float)old[k];
            im2 = (float)new[k];
            n = 0;
            ims1 = 0.0f;
            ims2 = 0.0f;
            ki = ki0;
            for (i1 = i0; i1 < i2; i1++)
            {
                for (j1 = j0; j1 < j2; j1++)
                {
                    kj = ki + j1;
                    imf1 = (float)old[kj];
                    ims1 += imf1;
                    imf2 = (float)new[kj];
                    ims2 += imf2;
                    n++;
                }
                ki += width;
            }
            ims1 /= (float)n;
            ims2 /= (float)n;
            im1 *= 2.0f;
            im1 -= ims1;
            im1 -= sum1;
            im2 *= 2.0f;
            im2 -= ims2;
            im2 -= sum2;
            q12 = (im1 * im2);
            q12 = (q12 < 0.0f) ? -q12 : q12;
            sum12l += q12;
            sumq1l += (im1 * im1);
            sumq2l += (im2 * im2);
            k++;
        }
        sum12 += sum12l;
        sumq1 += sumq1l;
        sumq2 += sumq2l;
    }
    sumq = sqrt(sumq1 * sumq2);
    if (sumq > 0.0f)
    {
        cor = sum12 / sumq;
    } else {
        cor = (sumq1 == sumq2) ? 1.0f : 0.0f;
    }
    cor = (cor < 0.0f) ? -cor : cor;

    return cor;
}

/* SHARPENBAD(a,b,r) = (2* GR(a,r) * GR(b,r) + C) / (GR(a,r) * GR(a,r) + GR(b,r) * GR(b,r) + C)
 * GR(a,r) = a * n - E(a,r)
 * GR(b,r) = b * n - E(b,r)
 * C = 1 */
float metric_sharpenbad (uint8_t *inbuf, uint8_t *outbuf, int width, int height, int radius)
{
    uint8_t *ref, *cmp;
    int y, x, y0, x0, y1, x1, yf, xf, di, n;
    size_t k, ky0, kx0, kf0, kf;
    float sum1, sum2, ssl, ss, sql, sq, si, sharpenbad, c;

    ref = inbuf;
    cmp = outbuf;
    sharpenbad = 0.0f;
    c = 1.0f;
    if (radius > 0)
    {
        ss = 0.0f;
        sq = 0.0f;
        k = 0;
        for (y = 0; y < height; y++)
        {
            y0 = y - radius;
            y0 = (y0 < 0) ? 0 : y0;
            y1 = y + radius + 1;
            y1 = (y1 < height) ? y1 : height;
            ky0 = y0 * width;
            ssl = 0.0f;
            sql = 0.0f;
            for (x = 0; x < width; x++)
            {
                x0 = x - radius;
                x0 = (x0 < 0) ? 0 : x0;
                x1 = x + radius + 1;
                x1 = (x1 < width) ? x1 : width;
                kx0 = x0;
                kf0 = ky0 + kx0;
                kf = kf0;
                sum1 = 0.0f;
                sum2 = 0.0f;
                n = 0;
                for (yf = y0; yf < y1; yf++)
                {
                    for (xf = x0; xf < x1; xf++)
                    {
                        sum1 -= (float)ref[kf];
                        sum2 -= (float)cmp[kf];
                        n++;
                        kf++;
                    }
                    kf0 += width;
                    kf = kf0;
                }
                n = (n > 0) ? n : 1;
                sum1 += (float)(n * (int)ref[k]);
                sum2 += (float)(n * (int)cmp[k]);
                ssl += (2.0f * sum1 * sum2 + c);
                sql += ((sum1 * sum1) + (sum2 * sum2) + c);
                k++;
            }
            ss += ssl;
            sq += sql;
        }
    }
    sharpenbad = ss / sq;

    return sharpenbad;
}

/* S(M) = 1 - sqrt(1 - M * M) */
float cor_sigma (float cor)
{
    float sigma;


    cor = (cor < 0.0f) ? -cor : cor;
    sigma = cor;
    if (cor > 1.0f)
    {
        cor = 1.0f / cor;
        sigma = 1.0f - sqrt(1.0f - cor * cor);
        sigma = 1.0f / sigma;
    } else {
        sigma = 1.0f - sqrt(1.0f - cor * cor);
    }

    return sigma;
}

int index_clamp (int i, int a, int b)
{
    int buf[3] = {a, i, b};
    return buf[(int)(i > a) + (int)(i > b)];
}

/* GR(a,1) */
int pix_sharpen3 (uint8_t *inbuf, int width, int height, int i, int j, int k)
{
    int res = 0, di0, di1, dj0, dj1;
    i = index_clamp (i, 0, height);
    j = index_clamp (j, 0, width);
    di0 = (i > 0) ? -width : 0;
    di1 = (i < (height - 1)) ? width : 0;
    dj0 = (j > 0) ? -1 : 0;
    dj1 = (j < (width - 1)) ? 1 : 0;
    res = inbuf[k];
    res <<= 3;
    res -= inbuf[k + dj0];
    res -= inbuf[k + dj1];
    res -= inbuf[k + di0];
    res -= inbuf[k + di1];
    res -= inbuf[k + di0 + dj0];
    res -= inbuf[k + di0 + dj1];
    res -= inbuf[k + di1 + dj0];
    res -= inbuf[k + di1 + dj1];

    return res;
}

/* NHW(a,b,r) = E((a - b) * (a - b) * (GR(a,r) - GR(b,r))) /  E(GR(a,r) - GR(b,r))
 * GR(a,r) = a * n - E(a,r)
 * GR(b,r) = b * n - E(b,r) */
float metric_nhw (uint8_t *inbuf, uint8_t *outbuf, int width, int height)
{
    uint8_t *old, *new;
    int i, j, res, resr, resd, scan, delta;
    float amount = 0.0f, significance = 0.0f, neatness;
    float amountl = 0.0f, significancel = 0.0f;

    old = inbuf;
    new = outbuf;

    scan = 0;
    for (i = 0; i < height; i++)
    {
        amountl = 0.0f;
        significancel = 0.0f;
        for (j = 0; j < width; j++)
        {
            res = pix_sharpen3 (new, width, height, i, j, scan);
            resr = pix_sharpen3 (old, width, height, i, j, scan);
            resd = abs(res - resr);
            if (resd > 0)
            {
                delta = abs((float)new[scan] - (float)old[scan]);
                if (delta > 0)
                {
                    amountl += (delta * delta) * resd;
                }
                significancel += resd;
            }
            scan++;
        }
        amount += amountl;
        significance += significancel;
    }
    neatness = (significance > 0.0f) ? (amount / significance) : 0.0f;
    neatness = sqrt(neatness);
    neatness /= 255.0f;

    return neatness;
}

char* libsmallfry_version (void)
{
    return (char*) LIBSMALLFRYVERSION;
}
