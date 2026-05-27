# AArch64 Edge Terminator Consumer Preservation Repair

## Goal

Repair prepared edge and terminator publication so later join-block consumers
can preserve values before out-of-SSA edge publication reuses the same physical
register for a different incoming value.

## Why This Exists

The scalar cast/ALU route classified the remaining `00204` mismatch as a
placement problem outside a cast-only repair. Prepared BIR already records the
semantic consumer move `%t35 -> %t45.byte_offset.i64`, but schedules it as a
before-instruction move in `vaarg.join.39`. By that point predecessor edge
publication has already repurposed `x13` for `%t49`, so scalar cast lowering has
no valid current `%t35` source.

The missing durable capability is an edge/terminator preservation contract:
when a successor consumer needs a predecessor value after the edge copy would
clobber its physical register, prepared planning must either place the
consumer move before the terminator edge publication or preserve the source in
a valid home that successor consumers can use.

## In Scope

- Prepared consumer-move placement that depends on predecessor terminator or
  edge-publication clobbers.
- Out-of-SSA edge publication ordering when the source register for a later
  successor consumer is reused for another incoming value.
- Shared prepared lookup facts needed by AArch64 lowering to consume preserved
  sources without raw BIR scans.
- Minimal AArch64 consumer changes only when they consume the new prepared
  preservation answer.

Likely owned implementation surfaces:

- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

## Out Of Scope

- Do not repair this by reloading `%lv.ap.24` in the join block; the va_list
  field has already been mutated.
- Do not add cast-only or ALU-only fallbacks that merely reject stale registers
  without providing a valid preserved source.
- Do not change AAPCS64 call staging, variadic layout constants, or va_list
  cursor writeback rules under this idea.
- Do not fold unrelated dispatch value-materialization, memory identity,
  comparison, or global-load repairs into this route.
- Do not accept the existing dirty `memory.cpp` or `dispatch_edge_copies.cpp`
  repairs as completed work unless a supervisor routes a coherent proven slice
  for them.

## Acceptance Criteria

- The `%t35 -> %t45.byte_offset.i64` consumer preservation gap is repaired by a
  general prepared edge/terminator placement rule, not by matching `00204` or
  specific temporary names.
- Prepared planning can express or expose that a successor consumer source must
  be preserved before predecessor edge publication overwrites its register.
- AArch64 lowering consumes the prepared preservation result and does not trust
  stale emitted-register mappings after edge publication changes register
  authority.
- The focused eight-test subset passes, including the pointer-select and
  variadic aggregate byte-copy probes, without weakening expectations.
- Any remaining failure is classified to a precise next owner before more
  implementation.

## Reviewer Reject Signals

- Reject named-case logic for `00204`, `myprintf`, `%t35`, `%t45`, `%t49`,
  `vaarg.join.39`, or `x13`.
- Reject raw BIR scans, same-block producer walks, or textual assembly
  suppression used as the durable source of edge preservation truth.
- Reject join-time reloads from mutable locals when the source value was
  required before a predecessor mutation or edge publication.
- Reject changes that only shuffle AArch64 instruction order while prepared
  planning still cannot describe the preservation requirement.
- Reject expectation downgrades, unsupported-test rewrites, helper renames, or
  classification-only notes claimed as capability progress.
- Reject broad rewrites of calls, memory, dispatch, ALU, or comparison lowering
  that are not needed to implement the edge/terminator preservation contract.
