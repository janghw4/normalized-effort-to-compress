# Normalized Effort-to-Compress

C implementation of Effort-to-Compress (ETC), with a Python wrapper and a helper for reference-based normalization.

ETC counts recursive pair substitutions until a sequence becomes constant. The C library computes raw ETC. The Python `normalize` function computes nETC or cETC from a supplied lower bound and reference mean. Reference means must be estimated separately.

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

On macOS, compile with `cc -std=c99 -O3 -dynamiclib src/etc.c -o build/libetc.dylib` and set `ETC_LIBRARY` to its absolute path.

Tested on Linux under WSL. Windows and macOS builds are untested.

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

nETC uses a certified global lower bound and the uniform ensemble mean at the observed length and alphabet size. cETC uses the corresponding anchors conditioned on the ordered symbol-count vector. A reference mean must exceed its lower bound. Values above one indicate ETC above the reference mean and are returned without clipping.

## Deterministic convention

1. Test whether the current sequence is constant. If so, stop.
2. Count nonoverlapping occurrences of each ordered adjacent pair by a left-to-right scan.
3. Choose the maximum-frequency pair. Break ties by the smallest second label, then the smallest first label.
4. Replace all selected occurrences with one fresh label larger than every existing label.
5. Repeat. If the maximum nonoverlapping frequency is one, add the current length minus one and stop exactly.

The Python wrapper maps integer labels to their ascending ranks while preserving the sequence order and the count associated with each label. Changing numerical label order can affect tie breaking and ETC.

## C API and limits

Include `src/etc.h` and link against the compiled library.

```c
int compute_etc(const int *seq, int length);
void compute_etc_batch(const int *seqs, const int *lengths,
                       int n, int *results);
```

`compute_etc` returns a nonnegative count or a negative error code: -1 for invalid input, -2 for allocation failure, and -3 for an unsupported numerical range. Constant sequences return zero. The API also returns zero for empty input.

The caller must provide valid buffers of the stated sizes. The batch function expects concatenated sequences and an output array with `n` entries.

Pair counts use a dense array. Memory grows quadratically with the largest active label, while two sequence buffers use linear memory. Recode sparse input labels to ascending ranks before using the C API directly. The implementation rejects a nonconstant input label span above 46,337 and returns a range error if pair-index arithmetic would overflow. Available memory can impose a lower limit.

## Tests

`tests/test_etc.py` compares the library with an independent Python implementation that explicitly counts each candidate pair and does not use the frequency-one shortcut. It exhaustively checks binary sequences through length 12 and ternary sequences through length 8, then checks fixed random and order-preserving sparse-label cases. It also tests edge cases, error returns, batch calls, and normalization.

## References

- N. Nagaraj, K. Balasubramanian, and S. Dey, “A new complexity measure for time series analysis and classification,” European Physical Journal Special Topics 222, 847–860 (2013).
- K. Balasubramanian, [Effort-To-Compress (ETC) measure using Non-Sequential Recursive Pair Substitution (NSRPS)](https://www.mathworks.com/matlabcentral/fileexchange/67665-effort-to-compress-etc-measure-using-non-sequential-recursive-pair-substitution-nsrps), MATLAB Central File Exchange, version 1.1.0.0 (2018).

The C implementation and supporting Python code are released under the MIT license. Obtain the reference MATLAB implementation from the link above.
