// ----------------------------------------------------------------------------
// Copyright 2020 ARM Ltd., Carsten Bormann
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------
#include "cbor_parser.h"

#include <stdio.h>
#include <inttypes.h>

int cbor_get_argument(const uint8_t** p, const uint8_t* end, uint64_t* n) {
    if (*p >= end) {
        RETURN_ERROR(CBOR_ERR_OVERRUN);
    }
    uint8_t iv = **p & ~CBOR_TYPE_MASK;
    if (iv >= 28){
        RETURN_ERROR(CBOR_ERR_INTEGER_ENCODING);
    }
    (*p)++;
    if (iv < 24) {
        *n = iv;
    } else {
        const uint8_t* uend = *p + (1 << (iv-24));
        if (uend > end) {
            RETURN_ERROR(CBOR_ERR_OVERRUN);
        }
        for (*n = 0; *p < uend; (*p)++) {
            *n = *n << 8 | **p;
        }
    }
    return CBOR_ERR_NONE;
}
int cbor_get_uint64(const uint8_t** p, const uint8_t* end, uint64_t* n) {
    uint8_t type = **p & CBOR_TYPE_MASK;
    if (type != CBOR_TYPE_UINT) {
        RETURN_ERROR(CBOR_ERR_TYPE_MISMATCH);
    }
    return cbor_get_argument(p, end, n);
}

int cbor_get_int64(const uint8_t** p, const uint8_t* end, int64_t* n) {
    uint8_t type = **p & CBOR_TYPE_MASK;
    if (type != CBOR_TYPE_NINT && type != CBOR_TYPE_UINT) {
        RETURN_ERROR(CBOR_ERR_TYPE_MISMATCH);
    }
    uint64_t uv;
    int rc = cbor_get_argument(p, end, &uv);
    if (rc != CBOR_ERR_NONE) {
        return rc;
    }
    if (uv >> 63 != 0) {
        RETURN_ERROR(CBOR_ERR_INTEGER_DECODE_OVERFLOW);
    }
    if (type == CBOR_TYPE_NINT) {
        *n = -1 - (int64_t)uv;
    } else {
        *n = uv;
    }
    return rc;
}

int cbor_extract_uint(const uint8_t **p, const uint8_t *end, cbor_value_t *val) {
    return cbor_get_uint64(p, end, &(val->u64));
}

int cbor_extract_int(const uint8_t **p, const uint8_t *end, cbor_value_t *val) {
    return cbor_get_int64(p, end, &(val->i64));
}

int cbor_extract_ref(const uint8_t **p, const uint8_t *end, cbor_value_t *val) {
    int rc = cbor_get_argument(p, end, &(val->ref.length));
    if (rc == CBOR_ERR_NONE) {
        val->ref.ptr = *p;
    }
    return rc;
}

/* superset of cbor_extract_ref */
int cbor_extract_stringref(const uint8_t **p, const uint8_t *end, cbor_value_t *val) {
    int rc = cbor_get_argument(p, end, &(val->ref.length));
    if (rc == CBOR_ERR_NONE) {
        val->ref.ptr = *p;
        if (end - (*p) <= val->ref.length) {
          (*p) += val->ref.length;
        } else {
          SET_ERROR(rc, CBOR_ERR_OVERRUN);
        }
    }
    return rc;
}

int cbor_extract_tag(const uint8_t **p, const uint8_t *end, cbor_value_t *val) {
    RETURN_ERROR(CBOR_ERR_UNIMPLEMENTED);
}

int cbor_extract_primitive(const uint8_t **p, const uint8_t *end, cbor_value_t *val) {
    val->primitive = (**p & (~CBOR_TYPE_MASK));
    (*p)++;
    RETURN_ERROR(CBOR_ERR_NONE);
}

int cbor_check_type_extract_ref(const uint8_t **p, const uint8_t *end, cbor_value_t *o_val, const uint8_t cbor_type) {
    if ((**p & CBOR_TYPE_MASK) != cbor_type) {
        PD_PRINTF("Expected: %u Actual %u\n", (unsigned) cbor_type>>5, (unsigned)(**p & CBOR_TYPE_MASK)>>5);

        RETURN_ERROR(CBOR_ERR_TYPE_MISMATCH);
    }
    o_val->cbor_start = *p;
    return cbor_extract_ref(p, end, o_val);
}

int (*cbor_extractors[])(const uint8_t **p, const uint8_t *end, cbor_value_t *val) = {
    cbor_extract_uint,
    cbor_extract_int,
    cbor_extract_stringref,
    cbor_extract_stringref,
    cbor_extract_ref,
    cbor_extract_ref,
    cbor_extract_tag,
    cbor_extract_primitive
};

int cbor_skip(const uint8_t **p, const uint8_t *end) {
    uint8_t ct = **p & CBOR_TYPE_MASK;
    size_t handler_index = ct >> 5;
    cbor_value_t val;
    int rc = cbor_extractors[handler_index](p, end, &val);
    if ((*p) > end) {
        SET_ERROR(rc, CBOR_ERR_OVERRUN);
    }
    if (rc != CBOR_ERR_NONE) {
        return rc;
    }
    switch (ct) {
        case CBOR_TYPE_UINT:
        case CBOR_TYPE_NINT:
        case CBOR_TYPE_TSTR:
        case CBOR_TYPE_BSTR:
        case CBOR_TYPE_SIMPLE:
            break;    /* argument only */
        case CBOR_TYPE_MAP:
            val.ref.length *= 2;
            // no break;
        case CBOR_TYPE_LIST:
            for (size_t count = val.ref.length; count && rc == CBOR_ERR_NONE; count--) {
                rc = cbor_skip(p, end);
            }
            break;
        default:
            SET_ERROR(rc, CBOR_ERR_UNIMPLEMENTED);
    }
    return rc;
}
