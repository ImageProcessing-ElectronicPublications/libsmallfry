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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "smallfry.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static float factor_psnr (uint8_t *orig, uint8_t *cmp, int orig_stride, int cmp_stride, int width, int height, uint8_t max)
{
    uint8_t *old, *new;
    float ret;
    int sum;
    int i, j;

    sum = 0;
    old = orig;
    new = cmp;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
            sum += (old[j] - new[j]) * (old[j] - new[j]);

        old += orig_stride;
        new += cmp_stride;
    }

    ret  = (float) sum / (float) (width * height);
    ret  = 10.0f * log10(65025.0f / ret);

    if (max > 128)
        ret /= 50.0f;
    else
        ret /= (0.0016f * (float) (max * max)) - (0.38f * (float) max + 72.5f);

    return MAX(MIN(ret, 1.0f), 0.0f);
}

static float factor_aae (uint8_t *orig, uint8_t *cmp, int orig_stride, int cmp_stride, int width, int height, uint8_t max)
{
    uint8_t *old, *new;
    float ret;
    float sum;
    float cfmax, cf;
    int i, j;
    int cnt;

    sum = 0.0;
    cnt = 0;
    old = orig;
    new = cmp;

    for (i = 0; i < height; i++)
    {
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
                sum += 1.0f;
            else if (calc > 2.0)
                sum += (calc - 2.0f) / (5.0f - 2.0f);
        }

        old += orig_stride;
        new += cmp_stride;
    }

    old = orig + 7 * orig_stride;
    new = cmp  + 7 * cmp_stride;

    for (i = 7; i < height - 2; i += 8)
    {
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
                sum += 1.0f;
            else if (calc > 2.0f)
                sum += (calc - 2.0f) / (5.0f - 2.0f);
        }

        old += 8 * orig_stride;
        new += 8 * cmp_stride;
    }

    ret = 1 - (sum / (float) cnt);

    if (max > 128)
        cfmax = 0.65f;
    else
        cfmax = 1.0f - 0.35f * (float) max / 128.0f;

    cf = MAX(cfmax, MIN(1.0f, 0.25f + (1000.0f * (float) cnt) / sum));

    return ret * cf;
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

float metric_sharpenbad (uint8_t *inbuf, uint8_t *outbuf, int width, int height)
{
    uint8_t *old, *new;
    float sharpenbad, exp1n, k332, k255p;
    float im1, im2, imf1, imf2, ims1, ims2, imd, imd1, imd2, imdc;
    float sumd, sumd1, sumd2, sumdc, sumdl, sumd1l, sumd2l, sumdcl;
    int i, j, i0, j0, i1, j1, i2, j2, k, ki0, ki, kj, n;

    old = inbuf;
    new = outbuf;
    exp1n = exp(-1);
    k332 = (3.0f * 3.0f * 2.0f + 1.0f) / (3.0f * 3.0f * 2.0f - 1.0f);
    k255p = 1.0f / 255.0f;

    k = 0;
    sumd = 0.0f;
    sumd1 = 0.0f;
    sumd2 = 0.0f;
    sumdc = 0.0f;
    for (i = 0; i < height; i++)
    {
        sumdl = 0.0f;
        sumd1l = 0.0f;
        sumd2l = 0.0f;
        sumdcl = 0.0f;
        i0 = i - 1;
        if (i0 < 0) {i0 = 0;}
        i2 = i + 2;
        if (i2 > height) {i2 = height;}
        ki0 = i0 * width;
        for (j = 0; j < width; j++)
        {
            j0 = j - 1;
            if (j0 < 0) {j0 = 0;}
            j2 = j + 2;
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
            imd1 = im1 - ims1;
            imd2 = im2 - ims2;
            im1 += imd1;
            im2 += imd2;
            imd = im1 - im2;
            imd1 *= k255p;
            imd2 *= k255p;
            imd *= k255p;
            imd *= imd;
            imdc = imd1 * imd2;
            imd1 *= imd1;
            imd2 *= imd2;
            sumdl += imd;
            sumd1l += imd1;
            sumd2l += imd2;
            sumdcl += imdc;
            k++;
        }
        sumd += sumdl;
        sumd1 += sumd1l;
        sumd2 += sumd2l;
        sumdc += sumdcl;
    }
    sumd2 *= sumd1;
    if (sumd2 > 0.0f)
    {
        sumd /= sumd2;
        sumd *= sumdc;
        sumd *= 2.0f;
    } else {
        sumd /= (float)height;
        sumd /= (float)width;
    }
    if (sumd < 0.0f) {sumd = -sumd;}
    sumd = sqrt(sumd);
    sumd = -sumd;
    sumd *= exp1n;
    sumd += k332;

    sharpenbad = sumd;

    return sharpenbad;
}

float metric_cor (uint8_t *inbuf, uint8_t *outbuf, int width, int height)
{
    uint8_t *old, *new;
    float im1, im2;
    float sum1, sum2, sum1l, sum2l;
    float sum12, sumq1, sumq2, sum12l, sumq1l, sumq2l, sumq, cor;
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
            sum12l += (im1 * im2);
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

float metric_corsharp (uint8_t *inbuf, uint8_t *outbuf, int width, int height, int radius)
{
    uint8_t *old, *new;
    float im1, im2, imf1, imf2, ims1, ims2;
    float sum1, sum2, sum1l, sum2l;
    float sum12, sumq1, sumq2, sum12l, sumq1l, sumq2l, sumq, cor;
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
            sum12l += (im1 * im2);
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
                delta = abs(new[scan] - old[scan]);
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
    neatness = (significance > 0.0f) ? (amount / significance) : 1.0f;

    return neatness;
}
