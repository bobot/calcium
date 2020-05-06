/*
    Copyright (C) 2020 Fredrik Johansson

    This file is part of Calcium.

    Calcium is free software: you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License (LGPL) as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.  See <http://www.gnu.org/licenses/>.
*/

#include "ca_qqbar.h"

int
ca_qqbar_express_in_field(fmpq_poly_t res, const ca_qqbar_t alpha, const ca_qqbar_t x, slong max_bits, int flags, slong prec)
{
    slong d, dx;
    acb_ptr vec;
    acb_t z;
    int found;

    d = ca_qqbar_degree(alpha);
    dx = ca_qqbar_degree(x);

    if (dx == 1)
    {
        fmpq_t t;
        fmpq_init(t);
        fmpz_neg(fmpq_numref(t), CA_QQBAR_COEFFS(x));
        fmpz_set(fmpq_denref(t), CA_QQBAR_COEFFS(x) + 1);
        fmpq_poly_set_fmpq(res, t);
        fmpq_clear(t);
        return 1;
    }

    found = 0;

    if (d % dx != 0 || (ca_qqbar_is_real(alpha) && !ca_qqbar_is_real(x)))
        return 0;

    acb_init(z);
    vec = _acb_vec_init(d + 1);
    ca_qqbar_get_acb(z, alpha, prec);
    _acb_vec_set_powers(vec, z, d, prec);
    ca_qqbar_get_acb(vec + d, x, prec);

    fmpq_poly_fit_length(res, d + 1);

    if (_ca_qqbar_acb_lindep(res->coeffs, vec, d + 1, 1, prec) &&
        !fmpz_is_zero(res->coeffs + d))
    {
        ca_qqbar_t v;

        fmpz_neg(res->den, res->coeffs + d);
        _fmpq_poly_set_length(res, d);
        _fmpq_poly_normalise(res);
        fmpq_poly_canonicalise(res);

        ca_qqbar_init(v);
        ca_qqbar_evaluate_fmpq_poly(v, res, alpha);
        found = ca_qqbar_equal(v, x);
        ca_qqbar_clear(v);
    }

    acb_clear(z);
    _acb_vec_clear(vec, d + 1);

    return found;
}

