# Legacy Compatibility Quarantine

Status: design contract; implementation has not started.

This directory is a temporary, read-only quarantine for explicitly named
legacy observations. It preserves migration evidence while the new typed
pipeline replaces old consumers. It is not a BIR stage, pass, analysis,
preparation product, allocator input, publication gate, or fallback backend.

## Closed manifest

The quarantine has no open-ended payload. Each admitted field requires one
manifest entry containing:

- a stable field ID and exact legacy source file/symbol;
- the typed stage view and exact revision key from which it is observed;
- one writer, a closed reader allowlist, and a test-only or migration-only
  build mode;
- whether the value is display, parity-counter, route-snapshot, lookup-trace,
  pointer/index observation, or debug-publication evidence;
- a deletion issue, the checkpoint at which its reader count decreases, and a
  terminal zero-field/zero-reader gate.

Production uses an empty manifest. Migration mode enables only individually
reviewed entries and records every read. Unknown fields, writers, readers,
build modes, or revision keys are rejected. A field cannot carry an editor,
stage object, analysis handle, mutable pointer, owning graph reference, or a
key that can be refreshed after capture.

## Quarantine boundary

All records are derived after the typed producer has completed. They may be
compared with legacy display names, route snapshots, lookup fallbacks,
pointer/index observations, and debug publication records for audit output.
They cannot be consulted by import acceptance, Raw/Canonical verification,
pass mutation, preparation, constraint binding, D1-D5 pseudo work, E1
liveness, E2 assignment, E3 spill/reload insertion, E4 publication, MIR
mapping/selection, or emission.

A missing typed fact is a producer gap and fails closed at its owning boundary.
The quarantine cannot create an opcode, type, ID, CFG edge, operand/result,
call or ABI fact, inline-asm role, constraint meaning, analysis result, abstract
home, spill object, target mapping, frame fact, relocation, or object byte. It
also cannot choose between competing typed results, change verifier severity,
or convert an unsupported/error result into success.

Names, textual signatures/initializers/arguments, rendered IR, pointer values,
container indices, old route agreement, and LLVM-compatible inline-asm text
remain observations. No new compiler contract may cite one of them as its
input. The original inline-asm source payload remains typed source data; only a
separate LLVM-compatible rendering belongs here.

## Revision and lifetime rules

Each record copies only bounded observation data and the complete originating
key. It never owns or extends the borrowed stage lifetime. A read supplies the
same key; mismatch, expiry, or ambiguity returns `QuarantineKeyMismatch` and no
value. Records cannot be projected from `RawBir` to `CanonicalBir`, between
Pseudo revisions, across an E3 retry, or from one `AllocatedBir` to another.

Audit comparison runs out of band after both sides exist. A mismatch produces
a migration diagnostic only. It cannot rerun a stage, mutate either side, or
select which result proceeds.

## Deletion discipline

Every checkpoint must report the manifest field count, writer count, reader
count, and reads by field ID. Counts may stay equal or decrease; adding a field
or reader requires an explicit architecture review and may not restore deleted
legacy behavior. A checkpoint is accepted only when all reads belong to the
allowlist and every retained entry still names a deletion issue.

The terminal state is an empty manifest, zero writers, zero readers, no
production linkage, and deletion of this directory. The coverage ledger owns
the disposition of legacy capability families; quarantine entries do not
count as accepted semantic owners.

## Required proof

- Compile production configuration with an empty manifest and no quarantine
  reader reachable from stage or backend code.
- Reject unknown fields/readers, stale keys, cross-revision projection, and
  attempts to store owning or mutable handles.
- Inject missing and mismatching observations and prove compilation results,
  stage products, fingerprints, and diagnostics from the typed producer remain
  unchanged.
- At each migration checkpoint, prove the allowlist and monotonically
  non-increasing field/reader counts, ending with the zero-field/zero-reader
  deletion gate.
