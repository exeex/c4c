# Target Pseudo Legalization and Optional Pass Gate

Status: converged deferred extension contract (unimplemented).

## D4 has one mandatory gate

`D4` consumes the exact immutable `PseudoBir` published by D3 together with
the matching `PseudoStageKey`, verified target layout, preparation bundle, and
constraint facts. Its required purpose is to make every non-`InlineAsm` node
directly realizable as exactly one machine instruction before out-of-SSA and
allocation. The legalization/expansion chain may contain no mutation only when
the target realizability checker proves that property for every input node.
The final full `Pseudo` reverification gate is never optional.

Required target legalization may replace one pseudo with an explicit pseudo
subgraph. Every introduced temporary, definition, use, constraint, clobber,
effect, stack-object reference, and CFG edit is ordinary BIR state visible to
out-of-SSA, liveness, and shared regalloc. The chain may legalize call-sequence
pseudos emitted by D2, but cannot redo ABI classification, choose a different
call plan, reconstruct a `GenericCall`, or take over general call lowering.

## Closed pass registry and optional entries

No pass is active merely because this directory exists. D4 uses a reviewed,
versioned ordered registry. Each entry declares:

- stable `PassId`, schema/options fingerprint, supported target fingerprints,
  exact predecessor properties, and admitted pseudo families;
- whether it is required legalization or an optional optimization, plus the
  typed feature/profile switch controlling an optional occurrence;
- deterministic rewrite and failure behavior, mutation budget, and exact
  postconditions;
- all analyses and derived products read, preserved with proof, invalidated,
  or recomputed; and
- the full-profile verifier re-entry point and direct-realizability checks.

Environment matching, pass-name strings, backend defaults, and local target
hooks cannot insert, omit, or reorder entries. An optional optimization may
change performance only: disabling it must still allow required legalization
and the final gate to establish the same schema, semantics, and realizability
contract.

## Mutation, invalidation, and revision rules

Every mutating occurrence forks a private candidate. Unchanged entities retain
stable IDs; a unique compatible replacement may retain its instruction ID;
additional nodes/results receive fresh IDs and removals become tombstones.
Every mutation advances the exact function/module revision and produces a new
`PseudoStageKey` with the ordered D4 occurrence fingerprint.

CFG edits invalidate CFG, dominance, SSA, def-use, liveness, and downstream
facts unless explicitly rebuilt. Value/instruction edits invalidate def-use,
SSA/dominance dependents, liveness, interference, assignment, realizability,
and any revision-bound projected constraints unless preservation is proved.
Constraint/effect edits additionally invalidate binding checks and clobber
facts. Pre-D4 liveness, interference, assignment, spill, and publication
products are never reusable after mutation.

Incremental checking may reject a bad edit early, but it cannot publish or
replace the final full gate. After the complete required-plus-enabled chain,
the runner freezes one exact candidate, recomputes every invalidated
prerequisite, and runs the entire cumulative `Pseudo` verifier plus target
realizability registry on every function. A mutation followed only by a local
check is not eligible for D5.

## Forbidden authority and failure

D4 may attach or refine abstract class/group requirements already authorized
by the verified layout, but it cannot assign ordinary values to homes, reserve
an undeclared unit, respond to pressure, insert capacity `Spill`/`Reload`,
encode a machine-register identity, select a stack displacement, or emit a
machine opcode. It cannot make a value disappear from allocation by burying it
in an opaque payload, implicit scratch convention, side table, or preassigned
operation. Any temporary is an ordinary allocatable value unless the closed
schema and target layout declare a fixed ABI requirement.

Unsupported operations, impossible expansion, missing scratch capacity,
invalid target facts, stale products, pass-budget exhaustion, nondeterminism,
cancellation, or failed reverification abort the whole D4 publication
transaction. No partial function set, pass result, property, revision, or
fallback revision is published. The D3 input remains immutable and is not
eligible for D5 unless a fresh complete D4 gate succeeds.
