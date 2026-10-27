#include "defuser.h"
#include <inttypes.h>
#include <stdio.h>

/* Restore every 32-bit operation with unsigned, modulo-2^32 arithmetic. */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32,
                            i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

static uint32_t rol5(uint32_t x) { return (x << 5) | (x >> 27); }
static uint32_t ror3(uint32_t x) { return (x >> 3) | (x << 29); }
static uint32_t ror5(uint32_t x) { return (x >> 5) | (x << 27); }

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t middle = (uint32_t)(challenge >> 16);
    uint32_t step1 = (high - middle) * 3U + 0x24e1U;
    uint32_t step0 = low ^ (high + 0xc12cU);
    uint32_t value = (step0 + step1 * 4U) ^ 0x778dU;
    return write_answer(out, cap, &value, 1);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[6];
    values[0] = (uint32_t)((challenge >> 40) & 63U) * 3U;
    for (unsigned i = 1; i < 6; ++i) {
        uint32_t part = (uint32_t)((challenge >> (16U + 5U * i)) & 15U);
        values[i] = (values[i - 1] * 2U ^ part) + 10U * i;
    }
    return write_answer(out, cap, values, 6);
}

static uint32_t phase3_dispatch(unsigned selector, uint32_t x) {
    /* Actual jump-table order: case_2, case_6, case_1, case_4,
       case_0, case_7, case_5, case_3. */
    switch (selector) {
        case 0: return (x * 4U) ^ 0xd27dU;
        case 1: return (x * 4U) ^ 0x6ee1U;
        case 2: return (x ^ 0x2238U) + (x >> 2);
        case 3: return x * 3U + 0xdd6bU;
        case 4: return x * 5U + 0x7185U;
        case 5: return x * 5U + 0x50faU;
        case 6: return (x ^ 0xef84U) + (x >> 1);
        case 7: return x * 5U + 0x31c18U;
        default: return 0;
    }
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[2];
    values[0] = (uint32_t)(challenge & 7U);
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[1] = phase3_dispatch(values[0], x);
    return write_answer(out, cap, values, 2);
}

static uint32_t phase4_walk(uint32_t target, uint32_t low, uint32_t high, uint32_t key) {
    uint32_t middle = low + (high - low) / 2U;
    if (target == middle) return (middle + 0x9eU) ^ key;
    if (target < middle) {
        uint32_t child = phase4_walk(target, low, middle - 1U, key);
        return (child * 3U) ^ (middle + key) ^ 0x1bU;
    }
    uint32_t child = phase4_walk(target, middle + 1U, high, key);
    return (child * 3U) ^ (middle + key) ^ 0x1aU;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t target = (uint32_t)(challenge & 127U);
    uint32_t key = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    uint32_t value = phase4_walk(target, 0, 127U, key);
    return write_answer(out, cap, &value, 1);
}

struct phase5_node { uint32_t value; uint32_t tag; unsigned next; };

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    /* Original nodes at 0x405ba0, stride 16. Index 12 represents NULL. */
    static const struct phase5_node nodes[12] = {
        {0x79bcU, 0x092cU, 4}, {0xf008U, 0x0847U, 5},
        {0x9480U, 0x051fU, 6}, {0xc0e1U, 0x030bU, 11},
        {0x9be0U, 0x066fU, 9}, {0x62d1U, 0x0b5fU, 3},
        {0xd269U, 0x0a71U, 0}, {0x2c5bU, 0x0181U, 2},
        {0x12aeU, 0x0761U, 7}, {0xeb19U, 0x042cU, 10},
        {0xa813U, 0x0029U, 12}, {0xe1bdU, 0x0259U, 8}
    };
    static const unsigned heads[8] = {1, 5, 3, 11, 8, 7, 2, 6};
    unsigned index = heads[(unsigned)(challenge & 7U)];
    unsigned steps = (unsigned)((challenge >> 3) & 3U);
    uint32_t value = (uint32_t)(challenge >> 6) ^
                     (uint32_t)(challenge >> 32) ^ 0xc315U;
    uint32_t tag = 0;
    for (unsigned i = 0; i <= steps; ++i) {
        if (index >= 12) return 1;
        const struct phase5_node *node = &nodes[index];
        tag = node->tag;
        value = (value * 3U + node->value) ^ tag;
        index = node->next;
    }
    uint32_t values[2] = {value, tag};
    return write_answer(out, cap, values, 2);
}

static uint32_t phase6_apply(unsigned selector, uint32_t x) {
    /* Actual table: op_2, op_3, op_1, op_5, op_2, op_1, op_0, op_4. */
    switch (selector) {
        case 0:
        case 4: return (x ^ (x >> 2)) + 0x9c95U;
        case 1: return (x ^ 0xb286U) + (x << 7);
        case 2:
        case 5: return (x + 0xbc47U) ^ (x >> 2);
        case 3: return ror3(x);
        case 6: return rol5(x) ^ 0x544eU;
        case 7: return (x * 2U + 0xaecbU) ^ (x * 7U);
        default: return 0;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    unsigned first = (unsigned)(challenge & 7U);
    unsigned second = (unsigned)((challenge >> 3) & 7U);
    uint32_t value = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    value = phase6_apply(first, value) ^ 0x43b6U;
    value = phase6_apply(second, value) + 0xd986U;
    return write_answer(out, cap, &value, 1);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t selector = (uint32_t)(context & 3U);
    uint32_t folded_challenge = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
    uint32_t folded_context = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t second = ror5(folded_challenge ^ folded_context) ^ 0x47d3U ^
                      (selector * 0xa562U) ^ (uint32_t)(context >> 16);
    uint32_t third = second ^ (folded_context + selector + 0xe2b1U +
                               (folded_challenge >> 5));
    /* Offset 32 holds marker 0x4e45504f (little-endian "OPEN").
       Offset 36 holds the three unsigned decimal fields. */
    int size = snprintf(out, cap,
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32
                        " %" PRIu32 " %" PRIu32,
                        selector, second, third);
    return size < 0 || (size_t)size >= cap;
}
