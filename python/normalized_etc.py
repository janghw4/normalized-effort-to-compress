"""Standard-library-only wrapper. Set ETC_LIBRARY to the compiled library."""
import ctypes
import math
import operator
import os
from pathlib import Path

_library = None
def _load():
    global _library
    if _library is None:
        default = Path(__file__).resolve().parents[1] / 'build' / ('etc.dll' if os.name == 'nt' else 'libetc.so')
        _library = ctypes.CDLL(os.environ.get('ETC_LIBRARY', str(default)))
        _library.compute_etc.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.c_int]
        _library.compute_etc.restype = ctypes.c_int
    return _library

def compute_etc(sequence):
    """Raw ETC with ascending-rank recoding that preserves numerical label order."""
    values = [operator.index(x) for x in sequence]
    if len(values) > 2**31-1:
        raise ValueError('Sequence length exceeds the C API limit')
    mapping = {v:i for i,v in enumerate(sorted(set(values)))}
    array = (ctypes.c_int * len(values))(*(mapping[v] for v in values))
    result = _load().compute_etc(array, len(values))
    if result < 0:
        raise RuntimeError({-1:'Invalid C input', -2:'C allocation failed', -3:'Dense pair array exceeds the supported range'}[result])
    return result

def normalize(etc, lower_bound, reference_mean):
    """(ETC - lower bound)/(reference mean - lower bound), without clipping.

    Use matched global anchors for nETC or matched labeled-composition anchors
    for cETC. This function does not estimate the reference mean.
    """
    if not all(math.isfinite(x) for x in (etc, lower_bound, reference_mean)):
        raise ValueError('All arguments must be finite')
    if etc < lower_bound or reference_mean <= lower_bound:
        raise ValueError('Require ETC >= lower bound and reference mean > lower bound')
    return (etc-lower_bound)/(reference_mean-lower_bound)
