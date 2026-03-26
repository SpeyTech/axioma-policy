/**
 * @file test_policy_totality.c
 * @brief L4 Policy Layer — Totality and Determinism Tests
 *
 * DVEC: v1.3
 * SRS-003 Coverage: 22 SHALL requirements
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "axilog/policy.h"
#include <stdio.h>
#include <string.h>

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL: %s (line %d)\n", msg, __LINE__); \
        g_tests_failed++; \
        return 0; \
    } \
} while(0)

#define TEST_ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        printf("  FAIL: %s — expected %d, got %d (line %d)\n", \
               msg, (int)(b), (int)(a), __LINE__); \
        g_tests_failed++; \
        return 0; \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(a, b, msg) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("  FAIL: %s (line %d)\n", msg, __LINE__); \
        g_tests_failed++; \
        return 0; \
    } \
} while(0)

#define RUN_TEST(fn) do { \
    printf("Running: %s\n", #fn); \
    if (fn()) { \
        printf("  PASS\n"); \
        g_tests_passed++; \
    } \
} while(0)

/* TEST 1: INITIALIZATION */
static int test_policy_init(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ct_fault_flags_t faults = {0};
    ax_policy_err_t err;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));
    agent.health = AX_HEALTH_ENABLED;

    err = ax_policy_init(&ctx, &ledger, &agent, &faults);

    TEST_ASSERT_EQ(err, AX_POLICY_OK, "init should succeed");
    TEST_ASSERT_EQ(ctx.breach_flag, 0, "breach_flag should be 0");
    TEST_ASSERT_EQ(ctx.evaluation_count, 0, "evaluation_count should be 0");

    return 1;
}

/* TEST 2: NULL CONTEXT */
static int test_null_context_handling(void)
{
    ct_fault_flags_t faults = {0};
    ax_policy_err_t err;

    err = ax_policy_init(NULL, NULL, NULL, &faults);

    TEST_ASSERT_EQ(err, AX_POLICY_ERR_NULL_CTX, "NULL ctx should fail");
    TEST_ASSERT_EQ(faults.domain, 1, "domain fault should be set");

    return 1;
}

/* TEST 3: STRING MAPPING */
static int test_string_mapping(void)
{
    const char *s;

    s = ax_policy_result_to_str(AX_POLICY_PERMITTED);
    TEST_ASSERT_STR_EQ(s, "PERMITTED", "PERMITTED string");

    s = ax_policy_result_to_str(AX_POLICY_BREACH);
    TEST_ASSERT_STR_EQ(s, "BREACH", "BREACH string");

    s = ax_policy_cmp_to_str(AX_POLICY_CMP_GT);
    TEST_ASSERT_STR_EQ(s, "GT", "GT string");

    return 1;
}

/* TEST 4: Q16.16 ARITHMETIC — NO UB */
static int test_q16_16_arithmetic(void)
{
    q16_16_t a, b;

    /* Test positive values */
    a = Q16_16_FROM_INT(70);
    TEST_ASSERT_EQ(a, 4587520, "70 in Q16.16");

    /* Test negative values (NO UB — uses multiplication) */
    a = Q16_16_FROM_INT(-100);
    TEST_ASSERT_EQ(a, -6553600, "-100 in Q16.16");

    a = Q16_16_FROM_INT(-1000);
    TEST_ASSERT_EQ(a, -65536000, "-1000 in Q16.16");

    /* Test comparisons */
    a = Q16_16_FROM_INT(75);
    b = Q16_16_FROM_INT(70);

    TEST_ASSERT(q16_16_gt(a, b), "75 > 70");
    TEST_ASSERT(!q16_16_lt(a, b), "75 not < 70");

    /* Test function version */
    a = q16_16_from_int(-50);
    TEST_ASSERT_EQ(a, -3276800, "q16_16_from_int(-50)");

    return 1;
}

/* TEST 5: SERIALIZATION FORMAT */
static int test_serialization_format(void)
{
    ax_policy_record_t record;
    char buf[AX_EVIDENCE_BUF_SIZE];
    ct_fault_flags_t faults = {0};
    int len;

    record.actual     = Q16_16_FROM_INT(75);
    record.ledger_seq = 42;
    record.policy_id  = "POL-001-MAX-VELOCITY";
    record.result     = AX_POLICY_BREACH;
    record.threshold  = Q16_16_FROM_INT(70);

    len = ax_policy_serialize(buf, sizeof(buf), &record, &faults);

    TEST_ASSERT(len > 0, "serialization should succeed");
    TEST_ASSERT(strstr(buf, "{\"actual\":") == buf, "actual should be first");
    TEST_ASSERT(strstr(buf, " ") == NULL, "no spaces");

    return 1;
}

/* TEST 6: TRUNCATION DETECTION */
static int test_serialization_truncation(void)
{
    ax_policy_record_t record;
    char buf[32];
    ct_fault_flags_t faults = {0};
    int len;

    record.actual     = Q16_16_FROM_INT(75);
    record.ledger_seq = 42;
    record.policy_id  = "POL-001-MAX-VELOCITY";
    record.result     = AX_POLICY_BREACH;
    record.threshold  = Q16_16_FROM_INT(70);

    len = ax_policy_serialize(buf, sizeof(buf), &record, &faults);

    TEST_ASSERT(len < 0, "truncation should fail");
    TEST_ASSERT_EQ(faults.serialise, 1, "serialise fault should be set");

    return 1;
}

/* TEST 7: SINGLE POLICY BREACH */
static int test_single_policy_breach(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_t     policy;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    ax_policy_result_t result;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    policy.policy_id  = AX_POLICY_ID_MAX_VELOCITY;
    policy.threshold  = Q16_16_FROM_INT(70);
    policy.comparison = AX_POLICY_CMP_GT;
    policy.enabled    = 1;

    input.value      = Q16_16_FROM_INT(75);
    input.ledger_seq = 1;

    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);

    TEST_ASSERT_EQ(result, AX_POLICY_BREACH, "75 > 70 should BREACH");
    TEST_ASSERT_EQ(ctx.breach_flag, 1, "breach_flag should be set");

    return 1;
}

/* TEST 8: SINGLE POLICY PERMITTED */
static int test_single_policy_permitted(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_t     policy;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    ax_policy_result_t result;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    policy.policy_id  = AX_POLICY_ID_MAX_VELOCITY;
    policy.threshold  = Q16_16_FROM_INT(70);
    policy.comparison = AX_POLICY_CMP_GT;
    policy.enabled    = 1;

    input.value      = Q16_16_FROM_INT(65);
    input.ledger_seq = 1;

    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);

    TEST_ASSERT_EQ(result, AX_POLICY_PERMITTED, "65 <= 70 should be PERMITTED");
    TEST_ASSERT_EQ(ctx.breach_flag, 0, "breach_flag should be 0");

    return 1;
}

/* TEST 9: BOUNDARY CONDITIONS */
static int test_boundary_conditions(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_t     policy;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    ax_policy_result_t result;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    policy.policy_id  = AX_POLICY_ID_MAX_VELOCITY;
    policy.threshold  = Q16_16_FROM_INT(70);
    policy.comparison = AX_POLICY_CMP_GT;
    policy.enabled    = 1;

    /* Exact boundary: 70 should be PERMITTED */
    input.value      = Q16_16_FROM_INT(70);
    input.ledger_seq = 1;

    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);
    TEST_ASSERT_EQ(result, AX_POLICY_PERMITTED, "70 == 70 should be PERMITTED for GT");

    return 1;
}

/* TEST 10: POLICY REGISTRY */
static int test_policy_registry(void)
{
    const ax_policy_registry_t *registry;
    size_t count;

    registry = ax_policy_get_registry();
    TEST_ASSERT(registry != NULL, "registry should not be NULL");

    count = ax_policy_get_count();
    TEST_ASSERT(count > 0, "registry should have policies");
    TEST_ASSERT(count <= AX_POLICY_MAX_COUNT, "count should not exceed max");

    return 1;
}

/* TEST 11: REGISTRY SORTED (CRITICAL — SRS-003-SHALL-012) */
static int test_registry_sorted(void)
{
    int sorted;

    sorted = ax_policy_verify_registry_sorted();

    TEST_ASSERT_EQ(sorted, 1, "registry must be sorted by policy_id");

    return 1;
}

/* TEST 12: EVALUATE ALL AGGREGATION */
static int test_evaluate_all_aggregation(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    ax_policy_result_t result;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    input.value      = Q16_16_FROM_INT(75);
    input.ledger_seq = 1;

    result = ax_policy_evaluate_all(&ctx, &input, &faults);

    TEST_ASSERT_EQ(result, AX_POLICY_BREACH, "overall should be BREACH");
    TEST_ASSERT_EQ(ctx.breach_flag, 1, "breach_flag should be set");

    return 1;
}

/* TEST 13: DETERMINISM */
static int test_determinism(void)
{
    ax_policy_record_t record;
    char buf1[AX_EVIDENCE_BUF_SIZE];
    char buf2[AX_EVIDENCE_BUF_SIZE];
    ct_fault_flags_t faults1 = {0};
    ct_fault_flags_t faults2 = {0};
    int len1, len2;

    record.actual     = Q16_16_FROM_INT(75);
    record.ledger_seq = 42;
    record.policy_id  = "POL-001-MAX-VELOCITY";
    record.result     = AX_POLICY_BREACH;
    record.threshold  = Q16_16_FROM_INT(70);

    len1 = ax_policy_serialize(buf1, sizeof(buf1), &record, &faults1);
    len2 = ax_policy_serialize(buf2, sizeof(buf2), &record, &faults2);

    TEST_ASSERT_EQ(len1, len2, "lengths should match");
    TEST_ASSERT_STR_EQ(buf1, buf2, "outputs should be identical");

    return 1;
}

/* TEST 14: TOTALITY — ALL Q16.16 VALUES */
static int test_totality(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    ax_policy_result_t result;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    /* Test edge cases including negative values (now safe — no UB) */
    q16_16_t test_values[] = {
        Q16_16_MIN,
        Q16_16_FROM_INT(-1000),
        Q16_16_FROM_INT(-1),
        0,
        1,
        Q16_16_FROM_INT(1),
        Q16_16_FROM_INT(50),
        Q16_16_FROM_INT(100),
        Q16_16_MAX
    };

    size_t num_values = sizeof(test_values) / sizeof(test_values[0]);

    for (size_t i = 0; i < num_values; i++) {
        ax_policy_reset_cycle(&ctx);
        memset(&faults, 0, sizeof(faults));

        input.value      = test_values[i];
        input.ledger_seq = (uint64_t)(i + 1);

        result = ax_policy_evaluate_all(&ctx, &input, &faults);

        TEST_ASSERT(result == AX_POLICY_PERMITTED || result == AX_POLICY_BREACH,
                   "result must be valid");
    }

    return 1;
}

/* TEST 15: EVIDENCE COUNT */
static int test_evidence_count(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    size_t policy_count;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    ledger.sequence = 0;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    policy_count = ax_policy_get_count();

    input.value      = Q16_16_FROM_INT(50);
    input.ledger_seq = 1;

    ax_policy_evaluate_all(&ctx, &input, &faults);

    TEST_ASSERT_EQ(ctx.evaluation_count, policy_count, "should evaluate all policies");
    TEST_ASSERT_EQ(ledger.sequence, policy_count, "ledger sequence should advance");

    return 1;
}

/* TEST 16: L5 INTEGRATION */
static int test_l5_integration(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    TEST_ASSERT_EQ(ax_policy_has_breach(&ctx), 0, "no breach initially");

    input.value      = Q16_16_FROM_INT(75);
    input.ledger_seq = 1;

    ax_policy_evaluate_all(&ctx, &input, &faults);

    TEST_ASSERT_EQ(ax_policy_has_breach(&ctx), 1, "breach after violation");

    ax_policy_reset_cycle(&ctx);

    TEST_ASSERT_EQ(ax_policy_has_breach(&ctx), 0, "no breach after reset");

    return 1;
}

/* TEST 17: COMPARISON OPERATORS */
static int test_comparison_operators(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_t     policy;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    ax_policy_result_t result;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    policy.policy_id = "TEST-CMP";
    policy.threshold = Q16_16_FROM_INT(50);
    policy.enabled   = 1;

    /* Test GT */
    policy.comparison = AX_POLICY_CMP_GT;
    input.value = Q16_16_FROM_INT(51);
    input.ledger_seq = 1;
    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);
    TEST_ASSERT_EQ(result, AX_POLICY_BREACH, "51 > 50 BREACH for GT");

    ax_policy_reset_cycle(&ctx);
    input.value = Q16_16_FROM_INT(50);
    input.ledger_seq = 2;
    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);
    TEST_ASSERT_EQ(result, AX_POLICY_PERMITTED, "50 == 50 PERMITTED for GT");

    /* Test LT */
    ax_policy_reset_cycle(&ctx);
    policy.comparison = AX_POLICY_CMP_LT;
    input.value = Q16_16_FROM_INT(49);
    input.ledger_seq = 3;
    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);
    TEST_ASSERT_EQ(result, AX_POLICY_BREACH, "49 < 50 BREACH for LT");

    /* Test GE */
    ax_policy_reset_cycle(&ctx);
    policy.comparison = AX_POLICY_CMP_GE;
    input.value = Q16_16_FROM_INT(50);
    input.ledger_seq = 4;
    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);
    TEST_ASSERT_EQ(result, AX_POLICY_BREACH, "50 >= 50 BREACH for GE");

    /* Test LE */
    ax_policy_reset_cycle(&ctx);
    policy.comparison = AX_POLICY_CMP_LE;
    input.value = Q16_16_FROM_INT(50);
    input.ledger_seq = 5;
    result = ax_policy_evaluate(&ctx, &policy, &input, &faults);
    TEST_ASSERT_EQ(result, AX_POLICY_BREACH, "50 <= 50 BREACH for LE");

    return 1;
}

/* TEST 18: CROSS-LAYER ORDERING */
static int test_cross_layer_ordering(void)
{
    ax_policy_ctx_t ctx;
    ax_ledger_ctx_t ledger;
    ax_agent_ctx_t  agent;
    ax_policy_input_t input;
    ct_fault_flags_t faults = {0};
    uint64_t initial_seq;
    size_t policy_count;

    memset(&ledger, 0, sizeof(ledger));
    ledger.state = AX_LEDGER_READY;
    ledger.sequence = 100;
    memset(&agent, 0, sizeof(agent));

    ax_policy_init(&ctx, &ledger, &agent, &faults);

    initial_seq = ledger.sequence;
    policy_count = ax_policy_get_count();

    input.value      = Q16_16_FROM_INT(50);
    input.ledger_seq = 1;

    ax_policy_evaluate_all(&ctx, &input, &faults);

    TEST_ASSERT_EQ(ledger.sequence, initial_seq + policy_count,
                  "ledger sequence should advance per policy");

    return 1;
}

int main(void)
{
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("axioma-policy Test Suite — L4 Policy Totality\n");
    printf("DVEC: v1.3 | SRS-003 v0.3 | 22 SHALL requirements\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");

    RUN_TEST(test_policy_init);
    RUN_TEST(test_null_context_handling);
    RUN_TEST(test_string_mapping);
    RUN_TEST(test_q16_16_arithmetic);
    RUN_TEST(test_serialization_format);
    RUN_TEST(test_serialization_truncation);
    RUN_TEST(test_single_policy_breach);
    RUN_TEST(test_single_policy_permitted);
    RUN_TEST(test_boundary_conditions);
    RUN_TEST(test_policy_registry);
    RUN_TEST(test_registry_sorted);
    RUN_TEST(test_evaluate_all_aggregation);
    RUN_TEST(test_determinism);
    RUN_TEST(test_totality);
    RUN_TEST(test_evidence_count);
    RUN_TEST(test_l5_integration);
    RUN_TEST(test_comparison_operators);
    RUN_TEST(test_cross_layer_ordering);

    printf("\n═══════════════════════════════════════════════════════════════════\n");
    printf("Results: %d passed, %d failed\n", g_tests_passed, g_tests_failed);
    printf("═══════════════════════════════════════════════════════════════════\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
