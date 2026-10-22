#ifndef NORMALIZED_ETC_H
#define NORMALIZED_ETC_H
#ifdef __cplusplus
extern "C" {
#endif
enum { ETC_INVALID_INPUT = -1, ETC_ALLOCATION_ERROR = -2, ETC_RANGE_ERROR = -3 };
/* Returns raw ETC, or a negative error code. Empty input returns 0 by API
 * convention. Nonempty constant input has ETC 0. Input is not modified.
 * seq must address at least length integers. Numeric label order matters.
 * Recode sparse labels to ascending ranks before calling this dense-array API.
 */
int compute_etc(const int *seq, int length);
/* Flattened concatenation. Caller supplies sum(lengths) integers and n outputs.
 * Each output is an ETC value or error code. A negative length invalidates
 * that item and all following items because their offsets are undefined.
 */
void compute_etc_batch(const int *seqs, const int *lengths, int n, int *results);
#ifdef __cplusplus
}
#endif
#endif
