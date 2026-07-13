# BIR Liveness and Interference Analysis

Status: converged design contract (unimplemented).

## E1 authority and input

`E1` is the sole shared liveness/interference analysis for RV64, AArch64, and
x86 allocation. It consumes one immutable, fully reverified D5 `PseudoBir`
revision, or one fully reverified E3 retry revision, plus the exact
`VerifiedTargetLayout` and `BoundConstraintSet` named by that revision. Its key
contains the complete pseudo stage stamp, module epoch and revision, ordered
function-revision digest, target and layout fingerprints, constraint-product
fingerprint, and E1 schema fingerprint. An equal graph, stable value IDs, or a
report from an earlier revision does not establish freshness.

E1 is analysis, not allocation policy. It does not rank homes, coalesce,
evict, select a spill candidate, place `Spill`/`Reload`, or mutate BIR.

## Exact facts

E1 uses the exhaustive BIR operand/definition visitor and terminator-derived
CFG. It computes block live-in/live-out sets, instruction-boundary liveness,
and one immutable interference product over every allocatable ordinary value,
D5 copy assignment identity, and E3 reload result. Phi and block-argument edge
uses are impossible at this boundary.

The product records:

- symmetric overlap interference, including loops and exceptional CFG shape;
- typed category/class/group requirements without choosing a home;
- assignment-equality ties and early-clobber exclusions from the exact bound
  constraint record;
- abstract alias-unit exclusions for inline-assembly clobbers and call
  boundaries, including values live across a call;
- D5 `EdgeCopy` and `ParallelCopy` boundary semantics: all bundle sources are
  read before any destination is written, destinations become live together,
  and only a noninterfering tied source/destination pair may share a home; and
- explicit `Spill`/`Reload` def-use and residency transitions on E3 retry
  revisions, with no hidden memory residency inferred by the analysis.

Layout data supplies the reviewed alias, class, group-width/alignment,
reserved-unit, and call-clobber rules. E1 has no architecture-name branches and
does not infer constraints from rendered operations or assembly bytes.

## Publication and invalidation

E1 publishes atomically only after complete function/module coverage,
symmetry, exact def-use coverage, valid copy-boundary modeling, valid
constraint/layout keys, and an unchanged frozen revision are proven. Missing
uses, stale keys, mixed revisions, unsupported groups, or an unrecognized
clobber fail closed and publish no partial facts.

Any graph, operand, definition, CFG, call, copy, constraint binding, target
layout, stage-stamp, or E3 spill/reload change invalidates the whole E1 product
and every E2 result derived from it. Preservation is allowed only for a
transaction that proves the complete key and all consumed identities are
unchanged; revision equality by itself is insufficient.
