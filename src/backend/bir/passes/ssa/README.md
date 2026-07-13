# SSA Canonicalization Pass

Status: scaffold.

Input: canonical CFG. Output: verified SSA BIR suitable for memory, aggregate,
and intrinsic passes.

Owns phi or block-argument normalization, incoming-edge repair through explicit
plans, trivial-phi elimination, SSA construction where required, and dominance
verification. Incoming keys are `BlockId`; values are `ValueId`. Names, labels,
vector positions, and rendered text are forbidden as identity.

Required analyses: CFG and dominance. The pass must specify whether each
subpass is single-run, idempotent, or fixed-point bounded.

Legacy coverage to review: `prealloc/out_of_ssa.cpp` only for discovering old
SSA assumptions; regalloc phi moves and parallel-copy behavior belong to the
later MIR `out_of_ssa` stage, not here.
