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
#ifndef _CBOR_PARSER_H_
#define _CBOR_PARSER_H_

#include <stdint.h>
#include <stddef.h>

#define CBOR_TYPE_UINT (0 << 5)
#define CBOR_TYPE_NINT (1 << 5)
#define CBOR_TYPE_BSTR (2 << 5)
#define CBOR_TYPE_TSTR (3 << 5)
#define CBOR_TYPE_LIST (4 << 5)
#define CBOR_TYPE_MAP (5 << 5)
#define CBOR_TYPE_TAG (6 << 5)
#define CBOR_TYPE_SIMPLE (7 << 5)

#define CBOR_TYPE_MASK (7 << 5)

#define CBOR_FALSE (CBOR_TYPE_SIMPLE | 20)
#define CBOR_TRUE (CBOR_TYPE_SIMPLE | 21)
#define CBOR_NULL (CBOR_TYPE_SIMPLE | 22)

#define PRINT_ON_ERROR 0

#define SET_ERROR(RC, VAL)\
    do{\
        (RC)=(VAL);\
        if((VAL) && PRINT_ON_ERROR){printf("Error " #VAL " (%i) set on %s:%u\r\n",(VAL),__FILE__,__LINE__);\
    }}while(0)

#define RETURN_ERROR(VAL)\
    do{\
        if((VAL) && PRINT_ON_ERROR){printf("Error " #VAL " (%i) set on %s:%u\r\n",(VAL),__FILE__,__LINE__);}\
        return (VAL);}while(0)


#ifdef PARSER_DEBUG
#define PD_PRINTF(...)\
    printf(__VA_ARGS__)
#else
#define PD_PRINTF(...)
#endif


#define ARRAY_SIZE(X) (sizeof(X)/sizeof((X)[0]))

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t vendor_id[16];
extern const uint8_t class_id[16];

enum {
    CBOR_ERR_NONE = 0,
    CBOR_ERR_TYPE_MISMATCH,
    CBOR_ERR_KEY_MISMATCH,
    CBOR_ERR_OVERRUN,
    CBOR_ERR_INTEGER_DECODE_OVERFLOW,
    CBOR_ERR_INTEGER_ENCODING,
    CBOR_ERR_UNIMPLEMENTED,
    CBOR_ERR_ENUM_END,                /* use this for continuation*/
};

typedef struct cbor_value_s {
    const uint8_t *cbor_start;
    union {
        uint64_t u64;
        int64_t i64;
        struct {
            const uint8_t *ptr;
            uint64_t length;
        } ref;
        uint8_t primitive;
    };
} cbor_value_t;

int cbor_get_as_uint64(const uint8_t** p, const uint8_t* end, uint64_t* n);
int cbor_get_uint64(const uint8_t** p, const uint8_t* end, uint64_t* n);
int cbor_get_int64(const uint8_t** p, const uint8_t* end, int64_t* n) ;

int cbor_extract_uint(const uint8_t **p, const uint8_t *end, cbor_value_t *val);
int cbor_extract_int(const uint8_t **p, const uint8_t *end, cbor_value_t *val);
int cbor_extract_ref(const uint8_t **p, const uint8_t *end, cbor_value_t *val);
int cbor_extract_tag(const uint8_t **p, const uint8_t *end, cbor_value_t *val); /* XXX */
int cbor_extract_primitive(const uint8_t **p, const uint8_t *end, cbor_value_t *val);
int cbor_check_type_extract_ref(const uint8_t **p, const uint8_t *end, cbor_value_t *o_val, const uint8_t cbor_type);

int cbor_skip(const uint8_t **p, const uint8_t *end);
#ifdef __cplusplus
}
#endif

#endif // _CBOR_PARSER_H_
