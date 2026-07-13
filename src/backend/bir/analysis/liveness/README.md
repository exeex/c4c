# BIR Liveness Analysis

Status: scaffold. Computes target-independent value liveness for semantic
queries only. Physical-register intervals and spill decisions belong to MIR.

Legacy coverage: `prealloc/liveness.*`, value-home consumers, interval inputs,
call-boundary liveness, and phi-edge uses. Review must separate semantic
liveness from allocator-specific numbering.
