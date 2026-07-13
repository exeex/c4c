# Inline Assembly Target Tables

Status: converged design contract (unimplemented).

## Contract and ownership boundary

Inline-assembly preparation is `C7`. It consumes the exact
`VerifiedPreparationInput` borrow, matching `VerifiedTargetLayout`, and the
published ABI, call, variadic, and address products. It reads no
assembly-template bytes. Its sole output is immutable target data used by the
next planner and by the later register-constraint stage.

`InlineAsmTargetTables` owns the target-admitted source vocabulary and maps
each admitted spelling to declarative category/class/group, role, width,
clobber-vocabulary, and eligibility rules. For the initial RV64 profile the
closed vocabulary is `r`, `=r`, `VR`, `VRM2`, `VRM4`, and `VRM8`, with table
entries sufficient to describe reviewed read/write, numeric-tie,
early-clobber, and clobber forms. `VRM1`, alternatives, named/fixed-register
operands, and unreviewed AArch64 or x86 spellings are absent and therefore
fail closed downstream.

This stage does not consume a particular instruction's constraint strings or
operand/result ordinals. It emits no typed use/result requirement, assignment
tie, early-clobber exclusion, or resolved clobber unit. The register-constraint
stage alone interprets descriptions and attaches meaning to ordinary values.
Opaque template and original constraint text remain unchanged.

Consequently C7 is tables-only: it cannot parse even an admitted spelling in a
particular instruction, inspect that instruction's description order, or bind
a table rule to an operand/result identity. C9 `bind_constraints` is the sole
interpreter and binder.

## Binding and consumers

The product key contains the complete Canonical `PipelineStageStamp`, exact
`TargetFingerprint`, layout and inline-asm-table schema fingerprints, and exact
ordered ABI/call/variadic/address fingerprints. Runtime-helper planning is the
immediate dependency consumer. The register-constraint stage consumes the
published table after the full preparation bundle is verified.

## Publication

One transaction constructs the complete vocabulary table. Duplicate spellings,
ambiguous declarative rules, references to absent layout classes/groups,
inconsistent widths or roles, unresolved abstract clobber vocabulary,
predecessor/key mismatch, or diagnostics publish no table. Canonical storage,
opaque texts, and all inputs remain unchanged.

Any change to the Canonical stage stamp, target fingerprint, layout/table
schema, or ABI/call/variadic/address predecessor fingerprint invalidates the
complete table and its C8/C9 successors. Changes to a particular original
constraint description do not change this target-only table, but they do
invalidate C9's `BoundConstraintSet`.

Legacy coverage: `prealloc/inline_asm.*`, stack-layout inline asm, regalloc
interaction, and every target emitter's inline-asm path. Legacy routines that
attach target meaning to instruction operands migrate to the sole later
interpreter, not into these tables.
