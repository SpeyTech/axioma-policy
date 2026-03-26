/**
 * @file policy.h
 * @brief L4 Policy Evaluation Layer — Certified Operational Envelope
 *
 * DVEC: v1.3
 * AXILOG DETERMINISM: D2 — Constrained Deterministic
 * MEMORY: Zero Dynamic Allocation
 *
 * SRS-003-SHALL-001: Policy evaluation D2 deterministic
 * SRS-003-SHALL-002: Q16.16 fixed-point arithmetic (no floating-point)
 * SRS-003-SHALL-003: Bit-identical AX:POLICY:v1 records
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 */

#ifndef AXILOG_POLICY_H
#define AXILOG_POLICY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY RESULT ENUMERATION
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    AX_POLICY_PERMITTED = 0,
    AX_POLICY_BREACH    = 1
} ax_policy_result_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY IDENTIFIERS (COMPILE-TIME CONSTANTS)
 *
 * SRS-003-SHALL-019: policy_id compile-time constant, globally unique, ASCII-only
 * SRS-003-SHALL-013: Policy set finite, compile-time, identical across builds
 *
 * INVARIANT: These MUST be in lexicographic order for deterministic evaluation.
 * The registry enforces this order; these macros define the canonical IDs.
 * ═══════════════════════════════════════════════════════════════════════════ */

#define AX_POLICY_ID_MAX_VELOCITY    "POL-001-MAX-VELOCITY"
#define AX_POLICY_ID_MIN_ALTITUDE    "POL-002-MIN-ALTITUDE"
#define AX_POLICY_ID_THERMAL_LIMIT   "POL-003-THERMAL-LIMIT"
#define AX_POLICY_ID_RATE_LIMIT      "POL-004-RATE-LIMIT"

/**
 * @brief Maximum number of policies in the registry
 *
 * SRS-003-SHALL-010: O(N) execution bound — N is bounded by this constant.
 */
#define AX_POLICY_MAX_COUNT          16

/**
 * @brief Maximum policy ID length (for buffer sizing)
 */
#define AX_POLICY_ID_MAX_LEN         32

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY COMPARISON TYPE
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    AX_POLICY_CMP_GT = 0,  /* actual > threshold → BREACH */
    AX_POLICY_CMP_LT = 1,  /* actual < threshold → BREACH */
    AX_POLICY_CMP_GE = 2,  /* actual >= threshold → BREACH */
    AX_POLICY_CMP_LE = 3   /* actual <= threshold → BREACH */
} ax_policy_cmp_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY DEFINITION STRUCTURE
 *
 * SRS-003-SHALL-019: policy_id MUST be const (immutable)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char     *policy_id;   /* MUST be const — compile-time constant */
    q16_16_t        threshold;   /* Q16.16 constraint value */
    ax_policy_cmp_t comparison;  /* Comparison operator */
    uint8_t         enabled;     /* Policy enabled flag */
} ax_policy_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY INPUT STRUCTURE
 *
 * SRS-003-SHALL-011: Input binding from AX:OBS:v1
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    q16_16_t  value;       /* Observed Q16.16 value */
    uint64_t  ledger_seq;  /* Ordering source from AX:OBS:v1 */
} ax_policy_input_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * AX:POLICY:v1 EVIDENCE RECORD
 *
 * SRS-003-SHALL-004: RFC 8785 (JCS) canonicalized
 * SRS-003-SHALL-005: Required fields
 * SRS-003-SHALL-018: Fields in lexicographic order
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    q16_16_t           actual;      /* Observed value */
    uint64_t           ledger_seq;  /* Input ordering source */
    const char        *policy_id;   /* Policy identifier (const) */
    ax_policy_result_t result;      /* Evaluation result */
    q16_16_t           threshold;   /* Policy constraint */
} ax_policy_record_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY EVALUATION CONTEXT
 *
 * SRS-003-SHALL-009: Zero dynamic memory allocation
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    ax_ledger_ctx_t  *ledger;            /* L6 audit substrate (borrowed) */
    ax_agent_ctx_t   *agent;             /* L5 agent context (borrowed) */
    uint8_t           breach_flag;       /* Set if ANY policy breached */
    uint8_t           evaluation_count;  /* Number of policies evaluated */
    uint64_t          last_eval_seq;     /* Last evaluated ledger_seq */
} ax_policy_ctx_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY REGISTRY STRUCTURE
 *
 * SRS-003-SHALL-013: Policy set finite, compile-time, identical across builds
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const ax_policy_t *policies;  /* Pointer to const array (immutable) */
    size_t             count;     /* Number of policies */
} ax_policy_registry_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * ERROR CODES
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    AX_POLICY_OK              =  0,
    AX_POLICY_ERR_NULL_CTX    = -1,
    AX_POLICY_ERR_NULL_INPUT  = -2,
    AX_POLICY_ERR_SERIALISE   = -3,
    AX_POLICY_ERR_COMMIT      = -4,
    AX_POLICY_ERR_INVALID_ID  = -5,
    AX_POLICY_ERR_OVERFLOW    = -6
} ax_policy_err_t;

/* ═══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 * ═══════════════════════════════════════════════════════════════════════════ */

ax_policy_err_t ax_policy_init(
    ax_policy_ctx_t  *ctx,
    ax_ledger_ctx_t  *ledger,
    ax_agent_ctx_t   *agent,
    ct_fault_flags_t *faults
);

ax_policy_result_t ax_policy_evaluate(
    ax_policy_ctx_t       *ctx,
    const ax_policy_t     *policy,
    const ax_policy_input_t *input,
    ct_fault_flags_t      *faults
);

ax_policy_result_t ax_policy_evaluate_all(
    ax_policy_ctx_t       *ctx,
    const ax_policy_input_t *input,
    ct_fault_flags_t      *faults
);

const ax_policy_registry_t* ax_policy_get_registry(void);
size_t ax_policy_get_count(void);

const char* ax_policy_result_to_str(ax_policy_result_t result);
const char* ax_policy_cmp_to_str(ax_policy_cmp_t cmp);

int ax_policy_serialize(
    char                     *buf,
    size_t                    size,
    const ax_policy_record_t *record,
    ct_fault_flags_t         *faults
);

int ax_policy_has_breach(const ax_policy_ctx_t *ctx);
void ax_policy_reset_cycle(ax_policy_ctx_t *ctx);

/**
 * @brief Verify policy registry is sorted (CI gate)
 *
 * SRS-003-SHALL-012: Evaluation order must be deterministic
 *
 * @return 1 if sorted, 0 if not sorted
 */
int ax_policy_verify_registry_sorted(void);

#ifdef __cplusplus
}
#endif

#endif /* AXILOG_POLICY_H */
