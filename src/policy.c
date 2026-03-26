/**
 * @file policy.c
 * @brief L4 Policy Evaluation Layer — Implementation
 *
 * DVEC: v1.3
 * AXILOG DETERMINISM: D2 — Constrained Deterministic
 * MEMORY: Zero Dynamic Allocation
 *
 * SRS-003 Coverage: 22 SHALL requirements
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 */

#include "axilog/policy.h"
#include <stdio.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * COMPILE-TIME POLICY REGISTRY
 *
 * SRS-003-SHALL-013: Policy set finite, compile-time, identical across builds
 * SRS-003-SHALL-019: policy_id compile-time constant, globally unique, ASCII-only
 * SRS-003-SHALL-009: Zero dynamic memory allocation
 * SRS-003-SHALL-012: Registry MUST be sorted by policy_id (lexicographic)
 *
 * INVARIANT: Policies are defined in lexicographic order by policy_id.
 *            ax_policy_verify_registry_sorted() validates this at startup/CI.
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Compile-time policy definitions (const, immutable)
 *
 * CRITICAL: These MUST be in lexicographic order by policy_id.
 * The ordering is verified by ax_policy_verify_registry_sorted().
 */
static const ax_policy_t g_policies[] = {
    /* POL-001 < POL-002 < POL-003 < POL-004 (lexicographic) */
    {
        .policy_id  = AX_POLICY_ID_MAX_VELOCITY,   /* "POL-001-MAX-VELOCITY" */
        .threshold  = Q16_16_FROM_INT(70),
        .comparison = AX_POLICY_CMP_GT,
        .enabled    = 1
    },
    {
        .policy_id  = AX_POLICY_ID_MIN_ALTITUDE,   /* "POL-002-MIN-ALTITUDE" */
        .threshold  = Q16_16_FROM_INT(100),
        .comparison = AX_POLICY_CMP_LT,
        .enabled    = 1
    },
    {
        .policy_id  = AX_POLICY_ID_THERMAL_LIMIT,  /* "POL-003-THERMAL-LIMIT" */
        .threshold  = Q16_16_FROM_INT(85),
        .comparison = AX_POLICY_CMP_GT,
        .enabled    = 1
    },
    {
        .policy_id  = AX_POLICY_ID_RATE_LIMIT,     /* "POL-004-RATE-LIMIT" */
        .threshold  = Q16_16_FROM_INT(1000),
        .comparison = AX_POLICY_CMP_GT,
        .enabled    = 1
    }
};

/**
 * @brief Policy count — compile-time constant
 *
 * SRS-003-SHALL-010: O(N) execution bound — N is bounded.
 */
#define G_POLICY_COUNT (sizeof(g_policies) / sizeof(g_policies[0]))

/* Static assert: policy count must not exceed maximum */
_Static_assert(G_POLICY_COUNT <= AX_POLICY_MAX_COUNT,
               "Policy count exceeds AX_POLICY_MAX_COUNT");

/**
 * @brief Global registry structure (const, immutable)
 */
static const ax_policy_registry_t g_policy_registry = {
    .policies = g_policies,
    .count    = G_POLICY_COUNT
};

/* ═══════════════════════════════════════════════════════════════════════════
 * STRING MAPPING
 *
 * SRS-003-SHALL-004: Canonical string representation
 * SRS-003-SHALL-008: Traceability
 * ═══════════════════════════════════════════════════════════════════════════ */

const char* ax_policy_result_to_str(ax_policy_result_t result)
{
    switch (result) {
        case AX_POLICY_PERMITTED: return "PERMITTED";
        case AX_POLICY_BREACH:    return "BREACH";
        default:                  return "UNKNOWN";
    }
}

const char* ax_policy_cmp_to_str(ax_policy_cmp_t cmp)
{
    switch (cmp) {
        case AX_POLICY_CMP_GT: return "GT";
        case AX_POLICY_CMP_LT: return "LT";
        case AX_POLICY_CMP_GE: return "GE";
        case AX_POLICY_CMP_LE: return "LE";
        default:               return "UNKNOWN";
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY ID COMPARISON (INTERNAL)
 *
 * SRS-003-SHALL-012: Evaluation order (policy_id ASC)
 * ═══════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Compare two policy IDs lexicographically
 *
 * @return Negative if a < b, zero if equal, positive if a > b
 */
static int ax_policy_id_compare(const char *a, const char *b)
{
    if (a == NULL && b == NULL) return 0;
    if (a == NULL) return -1;
    if (b == NULL) return +1;

    while (*a != '\0' && *b != '\0') {
        if ((unsigned char)*a < (unsigned char)*b) return -1;
        if ((unsigned char)*a > (unsigned char)*b) return +1;
        a++;
        b++;
    }

    if (*a == '\0' && *b == '\0') return 0;
    if (*a == '\0') return -1;
    return +1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * REGISTRY VERIFICATION
 *
 * SRS-003-SHALL-012: Verify registry is sorted (CI gate)
 * ═══════════════════════════════════════════════════════════════════════════ */

int ax_policy_verify_registry_sorted(void)
{
    size_t i;

    if (g_policy_registry.count <= 1) {
        return 1;  /* Trivially sorted */
    }

    for (i = 1; i < g_policy_registry.count; i++) {
        const char *prev = g_policy_registry.policies[i - 1].policy_id;
        const char *curr = g_policy_registry.policies[i].policy_id;

        if (ax_policy_id_compare(prev, curr) >= 0) {
            /* Not strictly ascending — violation */
            return 0;
        }
    }

    return 1;  /* Sorted */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * REGISTRY ACCESS
 * ═══════════════════════════════════════════════════════════════════════════ */

const ax_policy_registry_t* ax_policy_get_registry(void)
{
    return &g_policy_registry;
}

size_t ax_policy_get_count(void)
{
    return g_policy_registry.count;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * INITIALIZATION
 *
 * SRS-003-SHALL-001: Policy evaluation D2 deterministic
 * SRS-003-SHALL-009: Zero dynamic memory allocation
 * ═══════════════════════════════════════════════════════════════════════════ */

ax_policy_err_t ax_policy_init(
    ax_policy_ctx_t  *ctx,
    ax_ledger_ctx_t  *ledger,
    ax_agent_ctx_t   *agent,
    ct_fault_flags_t *faults)
{
    if (ctx == NULL) {
        if (faults != NULL) faults->domain = 1;
        return AX_POLICY_ERR_NULL_CTX;
    }

    ctx->ledger           = ledger;
    ctx->agent            = agent;
    ctx->breach_flag      = 0;
    ctx->evaluation_count = 0;
    ctx->last_eval_seq    = 0;

    return AX_POLICY_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * EVIDENCE SERIALIZATION
 *
 * SRS-003-SHALL-004: RFC 8785 (JCS) canonicalized
 * SRS-003-SHALL-018: Fields in lexicographic order
 * SRS-003-SHALL-008: Traceability
 * ═══════════════════════════════════════════════════════════════════════════ */

int ax_policy_serialize(
    char                     *buf,
    size_t                    size,
    const ax_policy_record_t *record,
    ct_fault_flags_t         *faults)
{
    int r;
    const char *result_str;

    /* Validate inputs */
    if (buf == NULL || record == NULL || size == 0) {
        if (faults != NULL) faults->domain = 1;
        return AX_POLICY_ERR_NULL_INPUT;
    }

    if (record->policy_id == NULL) {
        if (faults != NULL) faults->domain = 1;
        return AX_POLICY_ERR_INVALID_ID;
    }

    result_str = ax_policy_result_to_str(record->result);

    /*
     * SRS-003-SHALL-018: Fields in lexicographic order
     * Order: actual, ledger_seq, policy_id, result, threshold
     */
    r = snprintf(buf, size,
        "{\"actual\":%d,"
        "\"ledger_seq\":%llu,"
        "\"policy_id\":\"%s\","
        "\"result\":\"%s\","
        "\"threshold\":%d}",
        (int)record->actual,
        (unsigned long long)record->ledger_seq,
        record->policy_id,
        result_str,
        (int)record->threshold
    );

    /*
     * SNPRINTF VALIDATION (MANDATORY — DVEC §12)
     *
     * r < 0        → encoding error → domain fault
     * r >= size    → truncation → domain fault
     *
     * Partial JSON output is STRICTLY FORBIDDEN.
     */
    if (r < 0 || (size_t)r >= size) {
        if (faults != NULL) faults->serialise = 1;
        return AX_POLICY_ERR_SERIALISE;
    }

    return r;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICY COMPARISON (PURE FUNCTION)
 *
 * SRS-003-SHALL-002: Q16.16 fixed-point arithmetic
 * SRS-003-SHALL-017: Pure function (no hidden state)
 * SRS-003-SHALL-020: Policy totality (no undefined paths)
 * ═══════════════════════════════════════════════════════════════════════════ */

static int ax_policy_check_breach(
    q16_16_t        actual,
    q16_16_t        threshold,
    ax_policy_cmp_t cmp)
{
    /* SRS-003-SHALL-020: All paths defined, no undefined behavior */
    switch (cmp) {
        case AX_POLICY_CMP_GT: return q16_16_gt(actual, threshold) ? 1 : 0;
        case AX_POLICY_CMP_LT: return q16_16_lt(actual, threshold) ? 1 : 0;
        case AX_POLICY_CMP_GE: return q16_16_ge(actual, threshold) ? 1 : 0;
        case AX_POLICY_CMP_LE: return q16_16_le(actual, threshold) ? 1 : 0;
        default:               return 1; /* Unknown → BREACH (fail-safe) */
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * STUB: LEDGER COMMIT
 *
 * SRS-003-SHALL-016: Failure semantics (commit failure → BREACH)
 * ═══════════════════════════════════════════════════════════════════════════ */

static ax_policy_err_t ax_policy_commit_evidence(
    ax_policy_ctx_t  *ctx,
    const char       *evidence,
    size_t            evidence_len,
    ct_fault_flags_t *faults)
{
    (void)evidence;
    (void)evidence_len;
    (void)faults;

    if (ctx->ledger != NULL) {
        ctx->ledger->sequence++;
    }

    return AX_POLICY_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SINGLE POLICY EVALUATION
 *
 * SRS-003-SHALL-003: Deterministic evaluation
 * SRS-003-SHALL-015: Pre-commit invariant
 * SRS-003-SHALL-021: One AX:POLICY:v1 record per policy
 * SRS-003-SHALL-006: No L5 transition if BREACH
 * SRS-003-SHALL-007: BREACH forces L5 to ALARM or STOPPED
 * SRS-003-SHALL-010: O(N) execution bound
 * ═══════════════════════════════════════════════════════════════════════════ */

ax_policy_result_t ax_policy_evaluate(
    ax_policy_ctx_t       *ctx,
    const ax_policy_t     *policy,
    const ax_policy_input_t *input,
    ct_fault_flags_t      *faults)
{
    ax_policy_record_t record;
    ax_policy_result_t result;
    char evidence_buf[AX_EVIDENCE_BUF_SIZE];
    int  serialize_len;
    ax_policy_err_t commit_err;

    /* SRS-003-SHALL-020: Totality — all inputs produce valid result */
    if (ctx == NULL || policy == NULL || input == NULL) {
        if (faults != NULL) faults->domain = 1;
        return AX_POLICY_BREACH;
    }

    if (policy->policy_id == NULL) {
        if (faults != NULL) faults->domain = 1;
        return AX_POLICY_BREACH;
    }

    /* Evaluate using pure function */
    if (ax_policy_check_breach(input->value, policy->threshold, policy->comparison)) {
        result = AX_POLICY_BREACH;
    } else {
        result = AX_POLICY_PERMITTED;
    }

    /* Build evidence record */
    record.actual     = input->value;
    record.ledger_seq = input->ledger_seq;
    record.policy_id  = policy->policy_id;
    record.result     = result;
    record.threshold  = policy->threshold;

    /* Serialize to canonical JSON */
    serialize_len = ax_policy_serialize(evidence_buf, sizeof(evidence_buf), &record, faults);
    if (serialize_len < 0) {
        /* SRS-003-SHALL-016: Serialization failure → BREACH */
        return AX_POLICY_BREACH;
    }

    /* SRS-003-SHALL-015: Pre-commit invariant */
    commit_err = ax_policy_commit_evidence(ctx, evidence_buf, (size_t)serialize_len, faults);
    if (commit_err != AX_POLICY_OK) {
        /* SRS-003-SHALL-016: Commit failure → BREACH */
        if (faults != NULL) faults->commit = 1;
        return AX_POLICY_BREACH;
    }

    /* Update context */
    ctx->evaluation_count++;
    if (result == AX_POLICY_BREACH) {
        /* SRS-003-SHALL-006: Set breach flag for L5 enforcement */
        /* SRS-003-SHALL-007: BREACH forces L5 state change */
        ctx->breach_flag = 1;
    }

    return result;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * ALL POLICIES EVALUATION
 *
 * SRS-003-SHALL-012: Evaluation order (ledger_seq ASC, policy_id ASC)
 * SRS-003-SHALL-014: ANY BREACH → overall BREACH
 * SRS-003-SHALL-010: O(N) execution bound
 * SRS-003-SHALL-022: Cross-layer ordering invariant
 *
 * NOTE: Registry is pre-sorted by policy_id, so we iterate in order.
 * ═══════════════════════════════════════════════════════════════════════════ */

ax_policy_result_t ax_policy_evaluate_all(
    ax_policy_ctx_t       *ctx,
    const ax_policy_input_t *input,
    ct_fault_flags_t      *faults)
{
    const ax_policy_registry_t *registry;
    ax_policy_result_t overall_result;
    ax_policy_result_t policy_result;
    size_t i;

    /* SRS-003-SHALL-020: Totality */
    if (ctx == NULL || input == NULL) {
        if (faults != NULL) faults->domain = 1;
        return AX_POLICY_BREACH;
    }

    registry = ax_policy_get_registry();
    if (registry == NULL || registry->count == 0) {
        return AX_POLICY_PERMITTED;
    }

    /*
     * SRS-003-SHALL-012: Evaluation order
     *
     * Registry is pre-sorted by policy_id (lexicographic).
     * We iterate in array order, which IS the sorted order.
     * No runtime sorting needed — determinism guaranteed by registry invariant.
     */
    overall_result = AX_POLICY_PERMITTED;

    /* SRS-003-SHALL-010: O(N) bound — N <= AX_POLICY_MAX_COUNT */
    for (i = 0; i < registry->count; i++) {
        const ax_policy_t *policy = &registry->policies[i];

        /* Skip disabled policies */
        if (!policy->enabled) {
            continue;
        }

        /* SRS-003-SHALL-021: One record per policy */
        policy_result = ax_policy_evaluate(ctx, policy, input, faults);

        /* SRS-003-SHALL-014: ANY BREACH → overall BREACH */
        /* SRS-003-SHALL-022: Commits occur in order before L5 transition */
        if (policy_result == AX_POLICY_BREACH) {
            overall_result = AX_POLICY_BREACH;
        }

        /* Check for fatal faults */
        if (faults != NULL && ct_fault_any(faults)) {
            return AX_POLICY_BREACH;
        }
    }

    ctx->last_eval_seq = input->ledger_seq;

    return overall_result;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * L5 INTEGRATION HELPERS
 *
 * SRS-003-SHALL-006: No L5 transition if BREACH
 * SRS-003-SHALL-007: BREACH forces L5 to ALARM or STOPPED
 * ═══════════════════════════════════════════════════════════════════════════ */

int ax_policy_has_breach(const ax_policy_ctx_t *ctx)
{
    if (ctx == NULL) return 0;
    return ctx->breach_flag ? 1 : 0;
}

void ax_policy_reset_cycle(ax_policy_ctx_t *ctx)
{
    if (ctx == NULL) return;
    ctx->breach_flag      = 0;
    ctx->evaluation_count = 0;
}
