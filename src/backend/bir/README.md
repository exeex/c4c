# New BIR Architecture

Status: design-only scaffold; no implementation is authorized by this tree.

This directory describes the replacement backend as an ordered collection of
typed stages. The same mutable BIR storage is carried through canonical
BIR-to-BIR passes, but a verified stage token controls which operations are
legal. Derived facts are analyses, and target realization is kept outside the
canonical pass sequence.

## Required stage order

```text
LIR
  -> lir_to_bir
  -> RawBir verification
  -> canonical BIR pass pipeline
  -> CanonicalBir verification
  -> preparation planners
  -> PreparedBir
  -> MIR construction and MIR pass pipeline
  -> verified MIR
  -> target emission
```

The authoritative order and pass-to-pass contracts live in
[`pipeline/README.md`](pipeline/README.md). A pass may run only after its stated
preconditions hold. Re-running a pass is legal only when its document defines
idempotence or a fixed-point rule.

## Design areas

- [`passes/`](passes/README.md): target-independent semantic BIR mutations.
- [`analysis/`](analysis/README.md): immutable, revision-bound, recomputable
  facts used by passes and preparation.
- [`preparation/`](preparation/README.md): target-aware planners that read
  `CanonicalBir` without mutating it.
- [`mir/`](mir/README.md): stage transition and target realization passes.
- [`diagnostics/`](diagnostics/README.md): read-only diagnostics and rendering.
- [`compatibility/`](compatibility/README.md): temporary legacy quarantine.
- [`LEGACY_COVERAGE.md`](LEGACY_COVERAGE.md): migration ledger for every legacy
  backend capability family.
- [`REVIEW_TEMPLATE.md`](REVIEW_TEMPLATE.md): mandatory questions for repeated
  architecture review.

## Non-negotiable authority rules

1. Terminators are the only persistent CFG successor authority.
2. Stable IDs, not names, pointers, vector positions, or route numbers, are
   semantic identity.
3. Analyses never become persistent semantic side tables.
4. Canonical passes cannot read legacy compatibility state or target plans.
5. Preparation cannot mutate `CanonicalBir`.
6. Register assignment, spills, frame offsets, call moves, and target opcodes
   are not canonical BIR facts.
7. Each stage has one authoritative output and one verifier gate.


## Review state vocabulary

Every design document uses one of these states:

- `scaffold`: responsibility and boundaries exist, detailed behavior is open.
- `under-review`: legacy coverage and adjacent contracts are being checked.
- `accepted`: inputs, outputs, invariants, failure behavior, and legacy coverage
  have been reviewed together.
- `implemented`: allowed only after the entire ordered architecture is accepted.
