# `core/core.hpp` contract

## Legacy Evidence

The legacy x86 backend relied on wider utility files for assembly text
buffering and result carriage. This seam keeps the shared x86 support surface
small so higher layers do not depend on ad hoc helpers.

## Design Contract

This header is the single public header for `x86/core/`.

Owned inputs:

- staged assembly lines and raw text fragments
- final target-triple strings selected by higher-level owners
- emitted assembly text queried for symbol presence or definition

Owned outputs:

- `core::Text` for staged assembly text accumulation
- tiny symbol-query helpers for emitted text
- small shared result carriers used by public x86 entrypoints

Invariants:

- `core::Text` owns text accumulation mechanics only; sequencing and emission
  policy stay with callers
- `PublicResult` is a thin carrier for already-produced assembly text and
  target metadata, not a second orchestration layer
- symbol-query helpers are observational string queries over emitted text; they
  must not become parsers, validators, or relocation policy owners

Direct dependencies:

- the C++ standard library only; this public seam intentionally avoids target
  or backend-private include churn
- higher-level x86 owners such as `api/` and `module/` that supply staged text
  and consume `PublicResult`

Deferred behavior or upstream gaps:

- assembler, linker, and object-file ownership remain deferred subsystem work
- lowering policy, target admission, and module orchestration stay outside
  `core/`

Must not own:

- lowering policy
- target admission logic
- module orchestration
- assembler or linker side effects
