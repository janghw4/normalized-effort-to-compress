/*
 * Fast ETC (Effort-To-Compress) implementation in C.
 * Uses nonoverlapping counts and second-label-first numerical tie breaking.
 *
 * Optimizations:
 *  1. Phase II shortcut: mf=1 returns immediately
 *  2. Check exact constancy instead of calculating entropy
 *  3. Constant input returns zero without entering the substitution loop
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include "etc.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

static int is_constant(const int *seq, int L) {
    for (int i = 1; i < L; i++)
        if (seq[i] != seq[0]) return 0;
    return 1;
}

static int find_max_pair(const int *seq, int L, int max_sym,
                         int *pair_counts, int *best_a, int *best_b) {
    int dim = max_sym + 1;
    memset(pair_counts, 0, (size_t)dim * dim * sizeof(int));
    int i = 0;
    while (i < L - 1) {
        int a = seq[i], b = seq[i + 1];
        pair_counts[a * dim + b]++;
        if (a == b && i < L - 2 && seq[i + 2] == a)
            i++;
        i++;
    }
    int mc = 0;
    for (i = 0; i < dim * dim; i++)
        if (pair_counts[i] > mc) mc = pair_counts[i];
    if (mc == 0) return 0;
    *best_a = -1; *best_b = -1;
    for (int b = 0; b < dim && *best_a < 0; b++)
        for (int a = 0; a < dim; a++)
            if (pair_counts[a * dim + b] == mc) {
                *best_a = a; *best_b = b;
                goto found;
            }
found:
    return mc;
}

static int substitute(const int *seq, int L, int a, int b, int new_sym, int *out) {
    int j = 0, i = 0;
    while (i < L - 1) {
        if (seq[i] == a && seq[i + 1] == b) {
            out[j++] = new_sym;
            i += 2;
        } else {
            out[j++] = seq[i];
            i++;
        }
    }
    if (i == L - 1)
        out[j++] = seq[L - 1];
    return j;
}

EXPORT int compute_etc(const int *seq_in, int L) {
    if (L < 0 || (L > 0 && !seq_in)) return ETC_INVALID_INPUT;
    if (L == 0) return 0;
    if (L == 1) return 0;

    /* A constant sequence is already in the terminal state. */
    if (is_constant(seq_in, L)) return 0;

    if ((size_t)L > SIZE_MAX / sizeof(int)) return ETC_RANGE_ERROR;
    int *buf_a = (int *)malloc((size_t)L * sizeof(int));
    int *buf_b = (int *)malloc((size_t)L * sizeof(int));
    if (!buf_a || !buf_b) { free(buf_a); free(buf_b); return ETC_ALLOCATION_ERROR; }

    int min_val = seq_in[0];
    for (int i = 1; i < L; i++)
        if (seq_in[i] < min_val) min_val = seq_in[i];
    for (int i = 0; i < L; i++) {
        int64_t label = (int64_t)seq_in[i] - min_val;
        if (label > 46337) { free(buf_a); free(buf_b); return ETC_RANGE_ERROR; }
        buf_a[i] = (int)label;
    }

    int *cur = buf_a, *nxt = buf_b;
    int cur_L = L;
    int next_sym = 0;
    for (int i = 0; i < cur_L; i++)
        if (cur[i] > next_sym) next_sym = cur[i];
    next_sym++;

    size_t pc_cap = (size_t)(next_sym + 1) * (next_sym + 1);
    if (pc_cap > SIZE_MAX / sizeof(int)) { free(buf_a); free(buf_b); return ETC_RANGE_ERROR; }
    int *pair_counts = (int *)calloc(pc_cap, sizeof(int));
    if (!pair_counts) { free(buf_a); free(buf_b); return ETC_ALLOCATION_ERROR; }

    int N = 0;

    while (cur_L > 1) {
        /* Stop before pair counting if the preceding substitution was terminal. */
        if (is_constant(cur, cur_L)) break;

        if (next_sym > 46339) { N = ETC_RANGE_ERROR; break; }
        size_t needed = (size_t)(next_sym + 1) * (next_sym + 1);
        if (needed > pc_cap) {
            pc_cap = needed;
            if (pc_cap > SIZE_MAX / sizeof(int)) { N = ETC_RANGE_ERROR; break; }
            free(pair_counts);
            pair_counts = (int *)calloc(pc_cap, sizeof(int));
            if (!pair_counts) { N = ETC_ALLOCATION_ERROR; break; }
        }

        int ba, bb;
        int mc = find_max_pair(cur, cur_L, next_sym, pair_counts, &ba, &bb);
        if (mc <= 0) break;

        /* Phase II shortcut */
        if (mc == 1) {
            N += cur_L - 1;
            break;
        }

        int new_L = substitute(cur, cur_L, ba, bb, next_sym, nxt);
        int *tmp = cur; cur = nxt; nxt = tmp;
        cur_L = new_L;
        next_sym++;
        N++;
    }

    free(pair_counts);
    free(buf_a);
    free(buf_b);
    return N;
}

EXPORT void compute_etc_batch(const int *seqs, const int *lengths, int n, int *results) {
    if (n <= 0 || !results) return;
    if (!seqs || !lengths) {
        for (int i = 0; i < n; i++) results[i] = ETC_INVALID_INPUT;
        return;
    }
    size_t offset = 0;
    for (int i = 0; i < n; i++) {
        if (lengths[i] < 0 || (size_t)lengths[i] > SIZE_MAX - offset) {
            for (int j = i; j < n; j++) results[j] = ETC_INVALID_INPUT;
            return;
        }
        results[i] = compute_etc(seqs + offset, lengths[i]);
        offset += (size_t)lengths[i];
    }
}
