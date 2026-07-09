# Large Selected Pointer-Offset Local-Memory Policy

Status: Open
Type: Implementation
Parent: `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
Related:
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/closed/599_pointer_base_plus_offset_selected_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
Owning Layer: RV64 selected pointer offset materialization policy
Queue Order: 34
Prerequisites: selected pointer base+offset authority, memory-use freshness,
access width, and offset range facts must already be explicit
Proof Surface: large selected pointer-offset local-memory rows represented by
`src/ipa-sra-2.c` and `src/pr60822.c`

## Goal

Define how RV64 local-memory emission should handle selected pointer
base-plus-offset accesses whose offsets exceed the narrow immediate form,
without folding large-offset materialization into the generic frame-slot
consumer route.

## Why This Exists

Idea 614 consumed selected local-memory authority only for ordinary supported
frame-slot accesses. Step 4 residuals identified large selected pointer-offset
rows that need address materialization policy, scratch-register discipline, and
range handling separate from the narrow local-memory consumer.

## In Scope

- Refresh rows with large selected pointer offsets and verify that selected
  base, offset, width, address space, and freshness facts are complete.
- Define RV64 materialization for offsets outside immediate load/store range
  when scratch use and clobber constraints are explicit.
- Preserve fail-closed diagnostics for missing selected authority, unsupported
  offset ranges, absent scratch/clobber authority, or ambiguous pointer bases.
- Add focused positive and fail-closed tests for large selected pointer-offset
  local-memory access.

## Out Of Scope

- Producing selected pointer base-plus-offset authority.
- Direct pointer arithmetic policy without selected authority.
- String-constant and direct global-symbol local-memory policies.
- Aggregate stack-home policy, F128/16-byte width policy, ABI, runtime,
  expectations, unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe proves a shared large-offset local-memory family with
  complete selected authority.
- At least one large selected pointer-offset row moves past the current
  local-memory owner, or the route closes with a precise missing-authority or
  target-policy diagnosis.
- Negative proof keeps narrow frame-slot accesses, producer-owned pointer
  rows, direct pointer arithmetic, string/global rows, and aggregate homes
  outside this policy.

## Reviewer Reject Signals

- Reject testcase-shaped handling for `src/ipa-sra-2.c`, `src/pr60822.c`, or
  specific constant offsets from those files.
- Reject RV64 materialization that guesses selected pointer facts or scratch
  safety from final assembly shape.
- Reject weakening offset range, address-space, width, or freshness checks to
  make a narrow case pass.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave large-offset
  materialization unsupported behind a new label.
