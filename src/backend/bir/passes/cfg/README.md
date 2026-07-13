# CFG Canonicalization Pass

Status: scaffold.

Input: canonical scalar operations and valid terminators. Output: structurally
canonical control flow with terminators as the sole edge authority.

Candidate subpasses are unreachable removal, branch folding, block split/merge,
critical-edge preparation, and edge canonicalization. Every mutation uses
`BlockId` and transactional CFG editors and updates phi/block-argument edge
semantics atomically.

Required analyses: CFG and reachability; dominance only for subpasses that name
it. CFG-changing mutations invalidate CFG, dominance, loop, liveness, and SSA
analyses unless a narrower preservation proof exists.

Legacy coverage to review: route control-flow views, `prealloc/control_flow.hpp`,
labels, branch/switch/indirect edges, return paths, and edge publications.
