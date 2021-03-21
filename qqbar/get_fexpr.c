/*
    Copyright (C) 2021 Fredrik Johansson

    This file is part of Calcium.

    Calcium is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "acb_poly.h"
#include "fexpr_builtin.h"
#include "qqbar.h"

void
fexpr_set_arb(fexpr_t res, const arb_t x)
{
    fexpr_t mid, rad, h;
    arf_t r;
    fexpr_init(mid);
    fexpr_init(rad);
    fexpr_init(h);

    fexpr_set_symbol_builtin(h, FEXPR_RealBall);
    fexpr_set_arf(mid, arb_midref(x));
    arf_init_set_mag_shallow(r, arb_radref(x));
    fexpr_set_arf(rad, r);

    fexpr_call2(res, h, mid, rad);

    fexpr_clear(mid);
    fexpr_clear(rad);
    fexpr_clear(h);
}

void
fexpr_set_acb(fexpr_t res, const acb_t x)
{
    if (arb_is_zero(acb_imagref(x)))
    {
        fexpr_set_arb(res, acb_realref(x));
    }
    else if (arb_is_zero(acb_realref(x)))
    {
        fexpr_t v, i;
        fexpr_init(v);
        fexpr_init(i);
        fexpr_set_arb(v, acb_imagref(x));
        fexpr_set_symbol_builtin(i, FEXPR_NumberI);
        fexpr_mul(res, v, i);
        fexpr_clear(v);
        fexpr_clear(i);
    }
    else
    {
        fexpr_t r, v, i;
        fexpr_init(r);
        fexpr_init(v);
        fexpr_init(i);
        fexpr_set_arb(v, acb_imagref(x));
        fexpr_set_symbol_builtin(i, FEXPR_NumberI);
        fexpr_mul(r, v, i);
        fexpr_set_arb(v, acb_realref(x));
        fexpr_add(res, v, r);
        fexpr_clear(r);
        fexpr_clear(v);
        fexpr_clear(i);
    }
}

/* todo: better code (no temporaries) */
void
fexpr_set_list_fmpz_poly(fexpr_t res, const fmpz_poly_t poly)
{
    fexpr_struct * coeffs;
    fexpr_t t;
    slong i, len;

    len = poly->length;

    coeffs = _fexpr_vec_init(len);
    fexpr_init(t);

    for (i = 0; i < len; i++)
        fexpr_set_fmpz(coeffs + i, poly->coeffs + i);

    fexpr_set_symbol_builtin(t, FEXPR_List);
    fexpr_call_vec(res, t, coeffs, len);

    _fexpr_vec_clear(coeffs, len);
    fexpr_clear(t);
}

void
qqbar_get_fexpr_repr(fexpr_t res, const qqbar_t x)
{
    slong j, d;
    /* todo: better fexpr construction code */
    fexpr_struct * coeffs;
    fexpr_t t, u, v, w;

    d = qqbar_degree(x);

    coeffs = _fexpr_vec_init(d + 1);
    fexpr_init(t);
    fexpr_init(u);
    fexpr_init(v);
    fexpr_init(w);

    for (j = 0; j <= d; j++)
        fexpr_set_fmpz(coeffs + j, QQBAR_COEFFS(x) + j);

    fexpr_set_symbol_builtin(u, FEXPR_List);
    fexpr_call_vec(t, u, coeffs, d + 1);
    fexpr_set_symbol_builtin(v, FEXPR_AlgebraicNumberSerialized);

    fexpr_set_acb(u, QQBAR_ENCLOSURE(x));

    fexpr_call2(res, v, t, u);

    _fexpr_vec_clear(coeffs, d + 1);
    fexpr_clear(t);
    fexpr_clear(u);
    fexpr_clear(v);
    fexpr_clear(w);
}


void
_qqbar_get_fexpr_root_nearest(fexpr_t res, const fmpz_poly_t poly, const char * re_s, const char * im_s)
{
    fexpr_t Decimal, a, b, I, s;

    fexpr_init(Decimal);
    fexpr_init(a);
    fexpr_init(b);
    fexpr_init(I);
    fexpr_init(s);

    fexpr_set_symbol_builtin(Decimal, FEXPR_Decimal);

    if (re_s == NULL && im_s == NULL)
    {
        fexpr_set_string(s, "0.0");
        fexpr_call1(a, Decimal, s);
    }
    else
    {
        if (re_s != NULL)
        {
            fexpr_set_string(s, re_s);
            fexpr_call1(a, Decimal, s);
        }

        if (im_s != NULL)
        {
            fexpr_set_string(s, im_s);
            fexpr_call1(b, Decimal, s);
            fexpr_set_symbol_builtin(I, FEXPR_NumberI);
            fexpr_mul(s, b, I);
            fexpr_swap(b, s);
        }
    }

    if (im_s == NULL)
        fexpr_swap(s, a);
    else if (re_s == NULL)
        fexpr_swap(s, b);
    else
        fexpr_add(s, a, b);

    fexpr_set_list_fmpz_poly(b, poly);
    fexpr_set_symbol_builtin(a, FEXPR_PolynomialRootNearest);

    fexpr_call2(res, a, b, s);

    fexpr_clear(Decimal);
    fexpr_clear(a);
    fexpr_clear(b);
    fexpr_clear(I);
    fexpr_clear(s);
}

void
qqbar_get_fexpr_root_nearest(fexpr_t res, const qqbar_t x)
{
    acb_t z, point, delta;
    acb_poly_t poly;
    arb_t lhs, rhs, R, Rpow, tmpr;
    slong k, d, digits, prec, wp;
    char * re_s, * im_s;
    int success;
    int imag_zero, real_zero;

    d = qqbar_degree(x);

    /* For rational numbers, any numerical approximation will suffice */
    if (d == 1)
    {
        arb_t t;
        arb_init(t);
        digits = 6;
        qqbar_get_arb(t, x, 3.333 * digits + 10);
        re_s = arb_get_str(t, digits, ARB_STR_NO_RADIUS);
        _qqbar_get_fexpr_root_nearest(res, QQBAR_POLY(x), re_s, NULL);
        flint_free(re_s);
        arb_clear(t);
        return;
    }

    imag_zero = (qqbar_sgn_im(x) == 0);
    real_zero = (qqbar_sgn_re(x) == 0);

    /* For nonreal quadratics, getting the correct sign of the imaginary
       part (guaranteed by qqbar_get_acb) is sufficient. */
    if (d == 2 && !imag_zero)
    {
        acb_t t;
        acb_init(t);
        digits = 6;
        qqbar_get_acb(t, x, 3.333 * digits + 10);
        re_s = arb_get_str(acb_realref(t), digits, ARB_STR_NO_RADIUS);
        im_s = arb_get_str(acb_imagref(t), digits, ARB_STR_NO_RADIUS);
        _qqbar_get_fexpr_root_nearest(res, QQBAR_POLY(x), re_s, im_s);
        flint_free(re_s);
        flint_free(im_s);
        acb_clear(t);
        return;
    }

    acb_init(z);
    acb_init(point);
    acb_init(delta);
    acb_poly_init(poly);
    arb_init(lhs);
    arb_init(rhs);
    arb_init(R);
    arb_init(Rpow);
    arb_init(tmpr);

    acb_set(z, QQBAR_ENCLOSURE(x));
    if (imag_zero) arb_zero(acb_imagref(z));
    if (real_zero) arb_zero(acb_realref(z));

    re_s = im_s = NULL;

    success = 0;
    for (digits = 6; !success; digits *= 2)
    {
        prec = digits * 3.333 + 10;

        if (digits != 6)
            printf("digits %ld\n", digits);

        if (acb_rel_accuracy_bits(z) < prec)
        {
            _qqbar_enclosure_raw(z, QQBAR_POLY(x), z, prec);
            if (imag_zero) arb_zero(acb_imagref(z));
            if (real_zero) arb_zero(acb_realref(z));
        }

        flint_free(re_s);
        flint_free(im_s);
        re_s = arb_get_str(acb_realref(z), digits, ARB_STR_NO_RADIUS);
        im_s = arb_get_str(acb_imagref(z), digits, ARB_STR_NO_RADIUS);

        /* Let x be the true root and let z = a + bi be the approximate point. */
        /* Verify that D(z, C*|z-x|) contains a unique root, for some C > 1. */

        for (wp = prec; ; wp *= 2)
        {
            /* printf("inner loop %ld / %ld,  acc %ld\n", wp, prec, acb_rel_accuracy_bits(z)); */

            if (acb_rel_accuracy_bits(z) < wp)
            {
                _qqbar_enclosure_raw(z, QQBAR_POLY(x), z, wp);
                if (imag_zero) arb_zero(acb_imagref(z));
                if (real_zero) arb_zero(acb_realref(z));
            }

            arb_set_str(acb_realref(point), re_s, wp);
            arb_set_str(acb_imagref(point), im_s, wp);

            acb_sub(delta, z, point, wp);
            acb_abs(R, delta, wp);

            /* need more precision */
            if (arb_contains_zero(R))
            {
                success = 0;
                continue;
            }

            /* C = 1.25 */
            arb_mul_ui(R, R, 5, wp);
            arb_mul_2exp_si(R, R, -2);

            acb_poly_set_fmpz_poly(poly, QQBAR_POLY(x), wp);
            acb_poly_taylor_shift(poly, poly, point, wp);

            /* |a_1| R > (pi/3) (|a_0| + |a_2| R^2 + ... + |a_d| R^d) */
            acb_abs(lhs, poly->coeffs + 1, wp);
            arb_mul(lhs, lhs, R, wp);

            acb_abs(rhs, poly->coeffs, wp);

            arb_set(Rpow, R);
            for (k = 2; k <= d; k++)
            {
                arb_mul(Rpow, Rpow, R, wp);
                acb_abs(tmpr, poly->coeffs + k, wp);
                arb_addmul(rhs, tmpr, Rpow, wp);
            }

            arb_const_pi(tmpr, wp);
            arb_mul(rhs, rhs, tmpr, wp);
            arb_div_ui(rhs, rhs, 3, wp);

            if (arb_overlaps(lhs, rhs))
                continue;

            success = arb_gt(lhs, rhs);
            break;
        }
    }

    _qqbar_get_fexpr_root_nearest(res, QQBAR_POLY(x),
        real_zero ? NULL : re_s,
        imag_zero ? NULL : im_s);

    flint_free(re_s);
    flint_free(im_s);

    acb_clear(z);
    acb_clear(point);
    acb_clear(delta);
    acb_poly_clear(poly);
    arb_clear(lhs);
    arb_clear(rhs);
    arb_clear(R);
    arb_clear(Rpow);
    arb_clear(tmpr);
}

void
qqbar_get_fexpr_root_indexed(fexpr_t res, const qqbar_t x)
{
    qqbar_ptr conjugates;
    slong d, i, j;

    d = qqbar_degree(x);

    conjugates = qqbar_vec_init(d);
    qqbar_conjugates(conjugates, x);

    for (i = 0; i < d; i++)
    {
        if (qqbar_equal(conjugates + i, x))
        {
            /* todo: better fexpr construction code */
            fexpr_struct * coeffs;
            fexpr_t t, u, v;

            coeffs = _fexpr_vec_init(d + 1);
            fexpr_init(t);
            fexpr_init(u);
            fexpr_init(v);

            for (j = 0; j <= d; j++)
                fexpr_set_fmpz(coeffs + j, QQBAR_COEFFS(x) + j);

            fexpr_set_symbol_builtin(u, FEXPR_List);
            fexpr_call_vec(t, u, coeffs, d + 1);
            fexpr_set_si(u, i + 1);
            fexpr_set_symbol_builtin(v, FEXPR_PolynomialRootIndexed);
            fexpr_call2(res, v, t, u);

            _fexpr_vec_clear(coeffs, d + 1);
            fexpr_clear(t);
            fexpr_clear(u);
            fexpr_clear(v);

            break;
        }
    }

    qqbar_vec_clear(conjugates, d);
}

int
qqbar_get_fexpr_formula(fexpr_t res, const qqbar_t x, ulong flags)
{
    slong d;

    d = qqbar_degree(x);

    if (d == 1)
    {
        fmpq_t r;
        fmpz_t t;
        fmpq_init(r);
        fmpz_init(t);
        qqbar_get_quadratic(fmpq_numref(r), t, t, fmpq_denref(r), x, 0);
        fexpr_set_fmpq(res, r);
        fmpq_clear(r);
        fmpz_clear(t);
        return 1;
    }

    if (d <= 2)
    {
        fmpz_t a, b, c, q;
        fexpr_t t, u, v;

        fmpz_init(a);
        fmpz_init(b);
        fmpz_init(c);
        fmpz_init(q);
        fexpr_init(t);
        fexpr_init(u);
        fexpr_init(v);

        qqbar_get_quadratic(a, b, c, q, x, 2);

        if (fmpz_equal_si(c, -1))
        {
            fexpr_set_symbol_builtin(t, FEXPR_NumberI);
        }
        else
        {
            fexpr_set_fmpz(u, c);
            fexpr_set_symbol_builtin(v, FEXPR_Sqrt);
            fexpr_call1(t, v, u);
        }

        if (fmpz_is_zero(a))
        {
            if (fmpz_equal_si(b, -1))
                fexpr_neg(u, t);
            else if (fmpz_equal_si(b, 1))
                fexpr_swap(u, t);
            else
            {
                fexpr_set_fmpz(v, b);
                fexpr_mul(u, v, t);
            }
        }
        else
        {
            if (fmpz_equal_si(b, -1))
            {
                fexpr_set_fmpz(v, a);
                fexpr_sub(u, v, t);
            }
            else if (fmpz_equal_si(b, 1))
            {
                fexpr_set_fmpz(v, a);
                fexpr_add(u, v, t);
            }
            else
            {
                fexpr_set_fmpz(u, b);
                fexpr_mul(v, u, t);
                fexpr_set_fmpz(t, a);
                fexpr_add(u, t, v);
            }
        }

        if (fmpz_is_one(q))
        {
            fexpr_swap(res, u);
        }
        else
        {
            fexpr_set_fmpz(t, q);
            fexpr_div(res, u, t);
        }

        fmpz_clear(a);
        fmpz_clear(b);
        fmpz_clear(c);
        fmpz_clear(q);
        fexpr_clear(t);
        fexpr_clear(u);
        fexpr_clear(v);

        return 1;
    }

    /* Identify special constants */
    {
        slong p;
        ulong q;

        if (qqbar_is_root_of_unity(&p, &q, x))
        {
            fexpr_t s, t, u, v, w;
            /* exp(2 p pi i / q) */

            if (q % 2 == 0)
                q /= 2;
            else
                p *= 2;

            fexpr_init(s);
            fexpr_init(t);
            fexpr_init(u);
            fexpr_init(v);
            fexpr_init(w);

            fexpr_set_symbol_builtin(s, FEXPR_Mul);
            fexpr_set_si(t, p);
            fexpr_set_symbol_builtin(u, FEXPR_Pi);
            fexpr_set_symbol_builtin(v, FEXPR_NumberI);
            if (p == 1)
                fexpr_call2(w, s, u, v);
            else
                fexpr_call3(w, s, t, u, v);

            fexpr_set_si(t, q);
            fexpr_div(u, w, t);
            fexpr_set_symbol_builtin(t, FEXPR_Exp);
            fexpr_call1(res, t, u);

            fexpr_clear(s);
            fexpr_clear(t);
            fexpr_clear(u);
            fexpr_clear(v);
            fexpr_clear(w);

            return 1;
        }
    }

    return 0;
}