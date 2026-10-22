# Normalized Effort-to-Compress

C implementation of Effort-to-Compress (ETC), with a Python wrapper and a helper for reference-based normalization.

ETC counts recursive pair substitutions until a sequence becomes constant. It is not the compressed length or an entropy-rate estimator. The C library computes raw ETC. The Python `normalize` function applies supplied lower and mean anchors for nETC or cETC; it does not generate reference ensembles or estimate their means.

## Build

A C99 compiler is required. Python 3 is needed only for the wrapper and tests. The Python code has no third-party dependencies.

On Linux:

```sh
mkdir -p build
cc -std=c99 -O3 -fPIC -shared src/etc.c -o build/libetc.so
python3 tests/test_etc.py
```

Alternatively, with CMake:

```sh
cmake -S . -B build
cmake --build build --config Release
```

For a multi-configuration Windows build, point the wrapper to the output DLL:

```powershell
$env:ETC_LIBRARY = (Resolve-Path build/Release/etc.dll).Path
python tests/test_etc.py
```

On macOS, compile with `cc -std=c99 -O3 -dynamiclib src/etc.c -o build/libetc.dylib` and set `ETC_LIBRARY` to its absolute path. The current release was tested on Linux under WSL. Windows and macOS build instructions have not yet been exercised for this release.

## Python example

Run from the repository root:

```python
import sys
sys.path.insert(0, "python")
from normalized_etc import compute_etc, normalize

print(compute_etc([0, 1, 0, 1]))  # 1: the sequence becomes [2, 2]
print(compute_etc([7, 7, 7]))     # 0: already constant

# Illustrative anchors, not estimated reference values:
print(normalize(etc=5, lower_bound=1, reference_mean=3))  # 2.0
```

nETC uses a certified global lower bound and the uniform ensemble mean at the observed length and alphabet size. cETC uses the corresponding anchors conditioned on the ordered symbol-count vector. A reference mean must exceed its lower bound. Values are not clipped to [0, 1]; a value above one means ETC exceeds the chosen reference mean. Do not substitute the upper ceiling L-1 for either reference mean.

## Deterministic convention

1. Test whether the current sequence is constant. If so, stop.
2. Count nonoverlapping occurrences of each ordered adjacent pair by a left-to-right scan.
3. Choose the maximum-frequency pair. Break ties by the smallest second label, then the smallest first label.
4. Replace all selected occurrences with one fresh label larger than every existing label.
5. Repeat. If the maximum nonoverlapping frequency is one, add the current length minus one and stop exactly.

The frequency-one shortcut does not apply to every sequence. For example, `[0, 1, 0, 1]` becomes constant after a frequency-two substitution.

The Python wrapper maps integer labels to their ascending ranks. This preserves numerical label order, including for negative or sparse labels. It does not sort the sequence, sort composition counts, or relabel by frequency. Permuting the order of numerical labels can affect tie breaking and ETC.

## C API and limits

Include `src/etc.h` and link against the compiled library.

```c
int compute_etc(const int *seq, int length);
void compute_etc_batch(const int *seqs, const int *lengths,
                       int n, int *results);
```

`compute_etc` returns a nonnegative count or a negative error code: -1 for invalid input, -2 for allocation failure, and -3 for an unsupported numerical range. A nonempty constant sequence has ETC zero. Empty input returns zero as an API convention, not as a definition of the manuscript's ensembles.

The caller must provide valid buffers of the stated sizes. The batch function expects concatenated sequences and an output array with `n` entries. Raw pointer sizes cannot be checked by this API.

Pair counts use a dense array. Memory grows quadratically with the largest active label, while two sequence buffers use linear memory. Recode sparse input labels to ascending ranks before using the C API directly. The implementation rejects a nonconstant input label span above 46,337 and stops with a range error before its pair-index arithmetic can overflow. Allocation can fail at much smaller sizes. This is not an arbitrary-size or memory-optimal implementation.

## Validation and benchmark

`tests/test_etc.py` compares the library with an independent Python implementation that explicitly counts each candidate pair and does not use the frequency-one shortcut. It exhaustively checks binary sequences through length 12 and ternary sequences through length 8, then checks fixed random and order-preserving sparse-label cases. It also tests edge cases, error returns, batch calls, and normalization.

The manuscript implementation was separately compared with the reference MATLAB implementation on 16,607 sequences with zero mismatches. On an AMD Ryzen 9 9950X3D, median times over three runs were 0.0378 s for C and 2.6895 s for MATLAB (71.2-fold speedup; MATLAB startup excluded). The saved audit is in `validation/manuscript_matlab_parity.json`.

That timing belongs to the manuscript source identified by its SHA-256 in the audit. This public version adds input, allocation, and integer-range checks. It has separate parity tests, but the 71.2-fold timing has not been remeasured for this version. It is not a general speed guarantee.

## References

- N. Nagaraj, K. Balasubramanian, and S. Dey, “A new complexity measure for time series analysis and classification,” European Physical Journal Special Topics 222, 847–860 (2013).
- K. Balasubramanian, [Effort-To-Compress (ETC) measure using Non-Sequential Recursive Pair Substitution (NSRPS)](https://www.mathworks.com/matlabcentral/fileexchange/67665-effort-to-compress-etc-measure-using-non-sequential-recursive-pair-substitution-nsrps), MATLAB Central File Exchange, version 1.1.0.0 (2018).

The MATLAB code is not redistributed here. This repository contains the C implementation and supporting Python code under the MIT license. It does not include the manuscript's raw datasets or all analysis pipelines.
