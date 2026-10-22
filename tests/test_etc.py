"""Deterministic parity tests; no third-party Python dependencies."""
import ctypes
import itertools
import json
from pathlib import Path
import random
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'python'))
from normalized_etc import compute_etc, normalize, _load

def oracle(sequence):
    s=list(sequence); steps=0
    while len(set(s))>1:
        frequencies={}
        for pair in set(zip(s,s[1:])):
            i=count=0
            while i<len(s)-1:
                if (s[i],s[i+1])==pair: count+=1; i+=2
                else: i+=1
            frequencies[pair]=count
        a,b=min(frequencies,key=lambda pair:(-frequencies[pair],pair[1],pair[0]))
        fresh=max(s)+1; out=[]; i=0
        while i<len(s):
            if i+1<len(s) and (s[i],s[i+1])==(a,b): out.append(fresh); i+=2
            else: out.append(s[i]); i+=1
        s=out; steps+=1
    return steps

def main():
    count=0
    for k,max_length in [(2,12),(3,8)]:
        for length in range(1,max_length+1):
            for seq in itertools.product(range(k),repeat=length):
                assert compute_etc(seq)==oracle(seq),seq
                count+=1
    rng=random.Random(20261022)
    for k in [2,3,5,10,20]:
        for length in [20,100,500]:
            for trial in range(5):
                seq=[rng.randrange(k) for _ in range(length)]
                assert compute_etc(seq)==oracle(seq)
                assert compute_etc([x*100000-10000000000 for x in seq])==oracle(seq)
                count+=2
    assert compute_etc([])==0
    assert compute_etc([9]*100)==0
    assert compute_etc([0,1,0,1])==1
    lib=_load()
    assert lib.compute_etc(None,-1)==-1
    assert lib.compute_etc(None,2)==-1
    assert lib.compute_etc((ctypes.c_int*2)(-2147483648,2147483647),2)==-3
    lib.compute_etc_batch.argtypes=[ctypes.POINTER(ctypes.c_int),ctypes.POINTER(ctypes.c_int),ctypes.c_int,ctypes.POINTER(ctypes.c_int)]
    lib.compute_etc_batch.restype=None
    out=(ctypes.c_int*3)()
    lib.compute_etc_batch((ctypes.c_int*7)(0,1,0,1,1,2,3),(ctypes.c_int*3)(4,3,0),3,out)
    assert list(out)==[1,2,0]
    lib.compute_etc_batch((ctypes.c_int*1)(0),(ctypes.c_int*3)(1,-1,1),3,out)
    assert list(out)==[0,-1,-1]
    assert normalize(5,1,3)==2
    try: normalize(1,1,1)
    except ValueError: pass
    else: raise AssertionError('Degenerate denominator accepted')
    print(json.dumps({'status':'pass','oracle_comparisons':count,'additional_tests':'empty, constant, frequency-one, sparse labels, invalid input, range, batch, normalization'}))
if __name__=='__main__': main()
