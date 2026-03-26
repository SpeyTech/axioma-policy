/**
 * @file types.h
 * @brief Shared type definitions for Axioma framework
 *
 * DVEC: v1.3
 * AXILOG DETERMINISM: D1 — Strict Deterministic (type definitions)
 * MEMORY: Zero Dynamic Allocation
 *
 * SRS-001-SHALL-001: Contract Declaration
 * SRS-001-SHALL-002: Determinism Class Declaration
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 */

#ifndef AXILOG_TYPES_H
#define AXILOG_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * PLATFORM INVARIANTS (DVEC §12.3)
 *
 * These static assertions enforce platform assumptions at compile time.
 * Failure indicates a non-conformant target platform.
 * ═══════════════════════════════════════════════════════════════════════════ */

_Static_assert(sizeof(uint8_t)  == 1, "uint8_t must be 8-bit");
_Static_assert(sizeof(uint16_t) == 2, "uint16_t must be 16-bit");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be 32-bit");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t must be 64-bit");
_Static_assert(sizeof(int32_t)  == 4, "int32_t must be 32-bit");
_Static_assert(sizeof(int64_t)  == 8, "int64_t must be 64-bit");
_Static_assert(CHAR_BIT == 8, "CHAR_BIT must be 8");

/* ═══════════════════════════════════════════════════════════════════════════
 * FAULT CONTEXT (SRS-005-SHALL-001)
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Fault flags for deterministic error propagation
 *
 * SRS-005-SHALL-001: Persistent fault context
 * SRS-005-SHALL-002: Explicit fault propagation
 */
typedef struct {
    uint32_t overflow      : 1;
    uint32_t underflow     : 1;
    uint32_t div_zero      : 1;
    uint32_t domain        : 1;
    uint32_t serialise     : 1;
    uint32_t commit        : 1;
    uint32_t protocol      : 1;
    uint32_t io_error      : 1;
    uint32_t reserved      : 24;
} ct_fault_flags_t;

static inline int ct_fault_any(const ct_fault_flags_t *f) {
    return f->overflow || f->underflow || f->div_zero || f->domain ||
           f->serialise || f->commit || f->protocol || f->io_error;
}

static inline void ct_fault_clear(ct_fault_flags_t *f) {
    f->overflow = 0;
    f->underflow = 0;
    f->div_zero = 0;
    f->domain = 0;
    f->serialise = 0;
    f->commit = 0;
    f->protocol = 0;
    f->io_error = 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Q16.16 FIXED-POINT ARITHMETIC (SRS-003-SHALL-002)
 *
 * CRITICAL: No undefined behaviour permitted.
 * Left-shifting negative values is UB in C99 — we use multiplication instead.
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Q16.16 fixed-point type
 *
 * Format: 16 bits integer, 16 bits fractional
 * Range: -32768.0 to +32767.99998474
 * Resolution: 1/65536 ≈ 0.0000153
 */
typedef int32_t q16_16_t;

_Static_assert(sizeof(q16_16_t) == 4, "q16_16_t must be 32-bit");

#define Q16_16_FRAC_BITS    16
#define Q16_16_ONE          ((q16_16_t)65536)       /* 1.0 = 1 << 16 */
#define Q16_16_HALF         ((q16_16_t)32768)       /* 0.5 = 1 << 15 */
#define Q16_16_MAX          INT32_MAX
#define Q16_16_MIN          INT32_MIN

/**
 * @brief Convert integer to Q16.16 (SAFE — no UB)
 *
 * CRITICAL FIX: Uses multiplication instead of left-shift to avoid
 * undefined behaviour with negative values (C99 §6.5.7).
 *
 * The multiplication is performed in int64_t to prevent overflow,
 * then narrowed back to int32_t (q16_16_t).
 */
static inline q16_16_t q16_16_from_int(int32_t x) {
    return (q16_16_t)((int64_t)x * (int64_t)Q16_16_ONE);
}

/**
 * @brief Macro wrapper for compile-time constants
 *
 * SAFE: Uses multiplication, not shift.
 * Works for both positive and negative values.
 */
#define Q16_16_FROM_INT(x)  ((q16_16_t)((int64_t)(x) * (int64_t)65536))

/**
 * @brief Extract integer part from Q16.16
 *
 * Uses arithmetic right shift (implementation-defined but universal).
 * For maximum portability, use division instead if targeting exotic platforms.
 */
#define Q16_16_TO_INT(x)    ((int32_t)((x) / Q16_16_ONE))

/**
 * @brief Q16.16 comparison functions (no floating point)
 *
 * These are pure integer comparisons — deterministic across all platforms.
 */
static inline int q16_16_gt(q16_16_t a, q16_16_t b) { return a > b; }
static inline int q16_16_lt(q16_16_t a, q16_16_t b) { return a < b; }
static inline int q16_16_ge(q16_16_t a, q16_16_t b) { return a >= b; }
static inline int q16_16_le(q16_16_t a, q16_16_t b) { return a <= b; }
static inline int q16_16_eq(q16_16_t a, q16_16_t b) { return a == b; }

/* ═══════════════════════════════════════════════════════════════════════════
 * LEDGER CONTEXT (L6 SUBSTRATE)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    AX_LEDGER_UNINIT = 0,
    AX_LEDGER_READY  = 1,
    AX_LEDGER_FAILED = 2
} ax_ledger_state_t;

typedef struct {
    ax_ledger_state_t state;
    uint64_t          sequence;
    uint8_t           chain_head[32];
    uint8_t           genesis_hash[32];
} ax_ledger_ctx_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * AGENT TYPES (L5 BEHAVIOURAL LAYER)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    AX_HEALTH_UNINIT   = 0,
    AX_HEALTH_INIT     = 1,
    AX_HEALTH_ENABLED  = 2,
    AX_HEALTH_ALARM    = 3,
    AX_HEALTH_DEGRADED = 4,
    AX_HEALTH_STOPPED  = 5
} ax_health_state_t;

typedef enum {
    AX_INPUT_TIME_OBS       = 0,
    AX_INPUT_LLM_OBS        = 1,
    AX_INPUT_FAULT_SIGNAL   = 2,
    AX_INPUT_CLEAR_SIGNAL   = 3,
    AX_INPUT_ENABLE_CMD     = 4,
    AX_INPUT_DISABLE_CMD    = 5,
    AX_INPUT_RESET_CMD      = 6,
    AX_INPUT_POLICY_TRIGGER = 7
} ax_input_class_t;

typedef struct {
    ax_input_class_t  type;
    const uint8_t    *payload;
    uint64_t          payload_len;
    uint64_t          ledger_seq;
} ax_input_t;

typedef struct {
    ax_ledger_ctx_t   ledger;
    ax_health_state_t health;
    uint32_t          fault_accumulator;
    uint64_t          last_timestamp;
    uint64_t          last_seen_seq;
    uint8_t           local_failed_flag;
} ax_agent_ctx_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * BUFFER SIZES
 * ═══════════════════════════════════════════════════════════════════════════ */

#define AX_EVIDENCE_BUF_SIZE    512
#define AX_HASH_SIZE            32

#ifdef __cplusplus
}
#endif

#endif /* AXILOG_TYPES_H */
