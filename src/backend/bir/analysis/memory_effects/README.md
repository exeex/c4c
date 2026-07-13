# Memory and Effect Analysis

Status: scaffold. Classifies reads, writes, volatility, atomic ordering,
barriers, calls, and unknown effects from canonical instructions. Results are
recomputed after relevant operand, instruction, call, or CFG mutations.

Legacy coverage: memory access views, atomics, intrinsics, calls, and effect
assumptions currently embedded in lowering or prepared queries.
