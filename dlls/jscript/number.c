/*
 * Copyright 2008 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include <assert.h>

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;

    double value;
} NumberInstance;

static const WCHAR toStringW[] = {'t','o','S','t','r','i','n','g',0};
static const WCHAR toLocaleStringW[] = {'t','o','L','o','c','a','l','e','S','t','r','i','n','g',0};
static const WCHAR toFixedW[] = {'t','o','F','i','x','e','d',0};
static const WCHAR toExponentialW[] = {'t','o','E','x','p','o','n','e','n','t','i','a','l',0};
static const WCHAR toPrecisionW[] = {'t','o','P','r','e','c','i','s','i','o','n',0};
static const WCHAR valueOfW[] = {'v','a','l','u','e','O','f',0};

#define NUMBER_TOSTRING_BUF_SIZE 64
#define NUMBER_DTOA_SIZE 18

static inline NumberInstance *number_from_vdisp(vdisp_t *vdisp)
{
    return (NumberInstance*)vdisp->u.jsdisp;
}

static inline NumberInstance *number_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_NUMBER) ? number_from_vdisp(jsthis) : NULL;
}

static inline void dtoa(double d, WCHAR *buf, int size, int *dec_point)
{
    ULONGLONG l;
    int i;

    /* TODO: this function should print doubles with bigger precision */
    assert(size>=2 && size<=NUMBER_DTOA_SIZE && d>=0);

    if(d == 0)
        *dec_point = 0;
    else
        *dec_point = floor(log10(d));
    l = d*pow(10, size-*dec_point-1);

    if(l%10 >= 5)
        l = l/10+1;
    else
        l /= 10;

    buf[size-1] = 0;
    for(i=size-2; i>=0; i--) {
        buf[i] = '0'+l%10;
        l /= 10;
    }

    /* log10 was wrong by 1 or rounding changed number of digits */
    if(l) {
        (*dec_point)++;
        memmove(buf+1, buf, size-2);
        buf[0] = '0'+l;
    }else if(buf[0]=='0' && buf[1]>='1' && buf[1]<='9') {
        (*dec_point)--;
        memmove(buf, buf+1, size-2);
        buf[size-2] = '0';
    }
}

static inline void number_to_fixed(double val, int prec, BSTR *out)
{
    WCHAR buf[NUMBER_DTOA_SIZE];
    int dec_point, size, buf_size, buf_pos;
    BOOL neg = FALSE;
    BSTR str;

    if(val < 0) {
        neg = TRUE;
        val = -val;
    }

    if(val<=-1 || val>=1)
        buf_size = log10(val)+prec+2;
    else
        buf_size = prec+1;
    if(buf_size > NUMBER_DTOA_SIZE)
        buf_size = NUMBER_DTOA_SIZE;

    dtoa(val, buf, buf_size, &dec_point);
    dec_point++;
    size = 0;
    if(neg)
        size++;
    if(dec_point > 0)
        size += dec_point;
    else
        size++;
    if(prec)
        size += prec+1;

    str = SysAllocStringLen(NULL, size);
    size = buf_pos = 0;
    if(neg)
        str[size++] = '-';
    if(dec_point > 0) {
        for(;buf_pos<buf_size-1 && dec_point; dec_point--)
            str[size++] = buf[buf_pos++];
    }else {
        str[size++] = '0';
    }
    for(; dec_point>0; dec_point--)
        str[size++] = '0';
    if(prec) {
        str[size++] = '.';

        for(; dec_point<0 && prec; dec_point++, prec--)
            str[size++] = '0';
        for(; buf_pos<buf_size-1 && prec; prec--)
            str[size++] = buf[buf_pos++];
        for(; prec; prec--) {
            str[size++] = '0';
        }
    }
    str[size++] = 0;

    *out = str;
}

static inline void number_to_exponential(double val, int prec, BSTR *out)
{
    WCHAR buf[NUMBER_DTOA_SIZE], *pbuf;
    int dec_point, size, buf_size, exp_size = 1;
    BOOL neg = FALSE;
    BSTR str;

    if(val < 0) {
        neg = TRUE;
        val = -val;
    }

    buf_size = prec+2;
    if(buf_size<2 || buf_size>NUMBER_DTOA_SIZE)
        buf_size = NUMBER_DTOA_SIZE;
    dtoa(val, buf, buf_size, &dec_point);
    buf_size--;
    if(prec == -1)
        for(; buf_size>1 && buf[buf_size-1]=='0'; buf_size--)
            buf[buf_size-1] = 0;

    size = 10;
    while(dec_point>=size || dec_point<=-size) {
        size *= 10;
        exp_size++;
    }

    if(buf_size == 1)
        size = buf_size+2+exp_size; /* 2 = strlen(e+) */
    else if(prec == -1)
        size = buf_size+3+exp_size; /* 3 = strlen(.e+) */
    else
        size = prec+4+exp_size; /* 4 = strlen(0.e+) */
    if(neg)
        size++;
    str = SysAllocStringLen(NULL, size);

    size = 0;
    pbuf = buf;
    if(neg)
        str[size++] = '-';
    str[size++] = *pbuf++;
    if(buf_size != 1) {
        str[size++] = '.';
        while(*pbuf)
            str[size++] = *pbuf++;
        for(; prec>buf_size-1; prec--)
            str[size++] = '0';
    }
    str[size++] = 'e';
    if(dec_point >= 0) {
        str[size++] = '+';
    }else {
        str[size++] = '-';
        dec_point = -dec_point;
    }
    size += exp_size;
    do {
        str[--size] = '0'+dec_point%10;
        dec_point /= 10;
    }while(dec_point>0);
    size += exp_size;
    str[size] = 0;

    *out = str;
}

/* ECMA-262 3rd Edition    15.7.4.2 */
static HRESULT Number_toString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    NumberInstance *number;
    INT radix = 10;
    DOUBLE val;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_NUMBER_EXPECTED, NULL);

    if(arg_cnt(dp)) {
        hres = to_int32(ctx, get_arg(dp, 0), ei, &radix);
        if(FAILED(hres))
            return hres;

        if(radix<2 || radix>36)
            return throw_type_error(ctx, ei, JS_E_INVALIDARG, NULL);
    }

    val = number->value;

    if(radix==10 || isnan(val) || isinf(val)) {
        VARIANT v;

        num_set_val(&v, val);
        hres = to_string(ctx, &v, ei, &str);
        if(FAILED(hres))
            return hres;
    }else {
        INT idx = 0;
        DOUBLE integ, frac, log_radix = 0;
        WCHAR buf[NUMBER_TOSTRING_BUF_SIZE+16];
        BOOL exp = FALSE;

        if(val<0) {
            val = -val;
            buf[idx++] = '-';
        }

        while(1) {
            integ = floor(val);
            frac = val-integ;

            if(integ == 0)
                buf[idx++] = '0';
            while(integ>=1 && idx<NUMBER_TOSTRING_BUF_SIZE) {
                buf[idx] = fmod(integ, radix);
                if(buf[idx]<10) buf[idx] += '0';
                else buf[idx] += 'a'-10;
                integ /= radix;
                idx++;
            }

            if(idx<NUMBER_TOSTRING_BUF_SIZE) {
                INT beg = buf[0]=='-'?1:0;
                INT end = idx-1;
                WCHAR wch;

                while(end > beg) {
                    wch = buf[beg];
                    buf[beg++] = buf[end];
                    buf[end--] = wch;
                }
            }

            if(idx != NUMBER_TOSTRING_BUF_SIZE) buf[idx++] = '.';

            while(frac>0 && idx<NUMBER_TOSTRING_BUF_SIZE) {
                frac *= radix;
                buf[idx] = fmod(frac, radix);
                frac -= buf[idx];
                if(buf[idx]<10) buf[idx] += '0';
                else buf[idx] += 'a'-10;
                idx++;
            }

            if(idx==NUMBER_TOSTRING_BUF_SIZE && !exp) {
                exp = TRUE;
                idx = (buf[0]=='-') ? 1 : 0;
                log_radix = floor(log(val)/log(radix));
                val *= pow(radix, -log_radix);
                continue;
            }

            break;
        }

        while(buf[idx-1] == '0') idx--;
        if(buf[idx-1] == '.') idx--;

        if(exp) {
            if(log_radix==0)
                buf[idx] = 0;
            else {
                static const WCHAR formatW[] = {'(','e','%','c','%','d',')',0};
                WCHAR ch;

                if(log_radix<0) {
                    log_radix = -log_radix;
                    ch = '-';
                }
                else ch = '+';
                sprintfW(&buf[idx], formatW, ch, (int)log_radix);
            }
        }
        else buf[idx] = '\0';

        str = SysAllocString(buf);
        if(!str)
            return E_OUTOFMEMORY;
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Number_toLocaleString(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Number_toFixed(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    NumberInstance *number;
    DOUBLE val;
    INT prec = 0;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_NUMBER_EXPECTED, NULL);

    if(arg_cnt(dp)) {
        hres = to_int32(ctx, get_arg(dp, 0), ei, &prec);
        if(FAILED(hres))
            return hres;

        if(prec<0 || prec>20)
            return throw_range_error(ctx, ei, JS_E_FRACTION_DIGITS_OUT_OF_RANGE, NULL);
    }

    val = number->value;
    if(isinf(val) || isnan(val)) {
        VARIANT v;

        num_set_val(&v, val);
        hres = to_string(ctx, &v, ei, &str);
        if(FAILED(hres))
            return hres;
    }else {
        number_to_fixed(val, prec, &str);
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Number_toExponential(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    NumberInstance *number;
    DOUBLE val;
    INT prec = 0;
    BSTR str;
    HRESULT hres;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_NUMBER_EXPECTED, NULL);

    if(arg_cnt(dp)) {
        hres = to_int32(ctx, get_arg(dp, 0), ei, &prec);
        if(FAILED(hres))
            return hres;

        if(prec<0 || prec>20)
            return throw_range_error(ctx, ei, JS_E_FRACTION_DIGITS_OUT_OF_RANGE, NULL);
    }

    val = number->value;
    if(isinf(val) || isnan(val)) {
        VARIANT v;

        num_set_val(&v, val);
        hres = to_string(ctx, &v, ei, &str);
        if(FAILED(hres))
            return hres;
    }else {
        if(!prec)
            prec--;
        number_to_exponential(val, prec, &str);
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Number_toPrecision(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    NumberInstance *number;
    INT prec = 0, size;
    DOUBLE val;
    BSTR str;
    HRESULT hres;

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_NUMBER_EXPECTED, NULL);

    if(arg_cnt(dp)) {
        hres = to_int32(ctx, get_arg(dp, 0), ei, &prec);
        if(FAILED(hres))
            return hres;

        if(prec<1 || prec>21)
            return throw_range_error(ctx, ei, JS_E_PRECISION_OUT_OF_RANGE, NULL);
    }

    val = number->value;
    if(isinf(val) || isnan(val) || !prec) {
        VARIANT v;

        num_set_val(&v, val);
        hres = to_string(ctx, &v, ei, &str);
        if(FAILED(hres))
            return hres;
    }else {
        if(val != 0)
            size = floor(log10(val>0 ? val : -val)) + 1;
        else
            size = 1;

        if(size > prec)
            number_to_exponential(val, prec-1, &str);
        else
            number_to_fixed(val, prec-size, &str);
    }

    if(retv) {
        V_VT(retv) = VT_BSTR;
        V_BSTR(retv) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Number_valueOf(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    NumberInstance *number;

    TRACE("\n");

    if(!(number = number_this(jsthis)))
        return throw_type_error(ctx, ei, JS_E_NUMBER_EXPECTED, NULL);

    if(retv)
        num_set_val(retv, number->value);
    return S_OK;
}

static HRESULT Number_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    NumberInstance *number = number_from_vdisp(jsthis);

    switch(flags) {
    case INVOKE_FUNC:
        return throw_type_error(ctx, ei, JS_E_FUNCTION_EXPECTED, NULL);
    case DISPATCH_PROPERTYGET:
        num_set_val(retv, number->value);
        break;

    default:
        FIXME("flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_prop_t Number_props[] = {
    {toExponentialW,         Number_toExponential,         PROPF_METHOD|1},
    {toFixedW,               Number_toFixed,               PROPF_METHOD},
    {toLocaleStringW,        Number_toLocaleString,        PROPF_METHOD},
    {toPrecisionW,           Number_toPrecision,           PROPF_METHOD|1},
    {toStringW,              Number_toString,              PROPF_METHOD|1},
    {valueOfW,               Number_valueOf,               PROPF_METHOD}
};

static const builtin_info_t Number_info = {
    JSCLASS_NUMBER,
    {NULL, Number_value, 0},
    sizeof(Number_props)/sizeof(*Number_props),
    Number_props,
    NULL,
    NULL
};

static HRESULT NumberConstr_value(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei)
{
    double n;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case INVOKE_FUNC:
        if(!arg_cnt(dp)) {
            if(retv) {
                V_VT(retv) = VT_I4;
                V_I4(retv) = 0;
            }
            return S_OK;
        }

        hres = to_number(ctx, get_arg(dp, 0), ei, &n);
        if(FAILED(hres))
            return hres;

        if(retv)
            num_set_val(retv, n);
        break;

    case DISPATCH_CONSTRUCT: {
        jsdisp_t *obj;

        if(arg_cnt(dp)) {
            hres = to_number(ctx, get_arg(dp, 0), ei, &n);
            if(FAILED(hres))
                return hres;
        }else {
            n = 0;
        }

        hres = create_number(ctx, n, &obj);
        if(FAILED(hres))
            return hres;

        var_set_jsdisp(retv, obj);
        break;
    }
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT alloc_number(script_ctx_t *ctx, jsdisp_t *object_prototype, NumberInstance **ret)
{
    NumberInstance *number;
    HRESULT hres;

    number = heap_alloc_zero(sizeof(NumberInstance));
    if(!number)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&number->dispex, ctx, &Number_info, object_prototype);
    else
        hres = init_dispex_from_constr(&number->dispex, ctx, &Number_info, ctx->number_constr);
    if(FAILED(hres))
        return hres;

    *ret = number;
    return S_OK;
}

HRESULT create_number_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    NumberInstance *number;
    HRESULT hres;

    static const WCHAR NumberW[] = {'N','u','m','b','e','r',0};

    hres = alloc_number(ctx, object_prototype, &number);
    if(FAILED(hres))
        return hres;

    number->value = 0;
    hres = create_builtin_function(ctx, NumberConstr_value, NumberW, NULL,
            PROPF_CONSTR|1, &number->dispex, ret);

    jsdisp_release(&number->dispex);
    return hres;
}

HRESULT create_number(script_ctx_t *ctx, double value, jsdisp_t **ret)
{
    NumberInstance *number;
    HRESULT hres;

    hres = alloc_number(ctx, NULL, &number);
    if(FAILED(hres))
        return hres;

    number->value = value;

    *ret = &number->dispex;
    return S_OK;
}
