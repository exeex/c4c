# AArch64 Codegen Owner Layout Contraction Audit

## Goal

Re-run the post-cleanup AArch64 codegen layout audit after ideas 69-78 and
produce scoped follow-up ideas for the remaining file-count and line-count
residue under `src/backend/mir/aarch64/codegen`.

## Why This Exists

The previous call, memory, scalar, special-carrier, variadic, aggregate
transport, ordering, printer-status, and edge-copy authority routes consumed or
validated the major shared prepared facts. However, the AArch64 codegen layout
is still much larger than the reference ARM layout in
`ref/claudes-c-compiler/src/backend/arm/codegen`.

The remaining gap is no longer explained only by missing BIR or prealloc
authority. A large amount of code now lives in target-local consumer wrappers,
dispatch-publication helpers, edge-copy helpers, instruction-record surfaces,
and printer validation/spelling surfaces. Those need classification before any
implementation cleanup.

## Audit Findings

Current AArch64 codegen line-count concentration:

| Family | Approximate size | Classification |
| --- | ---: | --- |
| `calls.cpp` | 6805 | Prepared authority is consumed, but consumer wrappers and call-boundary record construction remain thick. |
| `memory.cpp` | 4790 | Prepared address/value-home authority is consumed, but memory-specific consumer wrappers and frame-slot helpers remain broad. |
| `alu.cpp` | 4306 | Shared scalar/control-flow authority is consumed, but scalar record construction and local operation helpers remain broad. |
| `dispatch*.cpp/.hpp` | 5805 | Dispatch-family cleanup reduced some duplication, but many publication and edge-copy helpers still sit in separate synthetic owners rather than in ref-style owner files. |
| `instruction.cpp/.hpp` + `machine_printer.cpp` | 6017 | Mostly target-local machine-record schema, status naming, validation, and printer spelling. Idea 77 found no evidence that this should move wholesale into BIR. |
| `globals.cpp`, `atomics.cpp`, `cast_ops.cpp`, `memory.cpp` | mixed | Idea 74 left same-shaped AArch64-local helper duplication here as an explicit follow-up candidate. |

Reference ARM codegen is much smaller because its owner split is coarse and
centralized around `emit.rs` plus small feature files. Our AArch64 route has
more prepared-authority consumer surfaces and a richer machine-record/printer
surface. Therefore the next cleanup should not blindly move more logic to BIR;
it should separate target-local layout contraction from genuine shared-authority
gaps.

## Classification

- `dispatch-publication-owner-residue`: target-local publication helpers are
  still split into `dispatch_publication.cpp`,
  `dispatch_value_materialization.cpp`, and `dispatch_producers.cpp`. Some are
  real dispatch route helpers, but others now look like owner-local emission
  helpers for memory, globals, scalar, compare, and fixed-formal store
  publication.
- `dispatch-edge-copy-owner-residue`: edge-copy lowering still has a dedicated
  large file. The shared source/stack facts have moved forward, so remaining
  code should be tested for fold-back into the dispatch owner or narrower
  consumer owners, not for another broad BIR migration.
- `record-printer-surface-residue`: `instruction.*` and `machine_printer.cpp`
  are mostly target-local schema/printer/validation. The likely cleanup is
  record-table or helper consolidation inside AArch64, not moving printer
  spelling into shared code.
- `local-helper-duplication-residue`: globals, atomics, cast, and memory owners
  repeat register-view, storage-plan, diagnostic, and prepared-record helper
  shapes. This is the remaining tail from idea 74.
- `prepared-consumer-wrapper-residue`: calls, memory, and ALU now consume shared
  facts, but the local consumer wrappers may be thicker than needed. This
  should be audited and contracted without re-deriving shared authority.

## Follow-Up Ideas Created

- `ideas/open/80_aarch64_dispatch_publication_owner_relocation.md`
- `ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md`
- `ideas/open/82_aarch64_instruction_printer_surface_contraction.md`
- `ideas/open/83_aarch64_local_helper_duplication_tail_cleanup.md`
- `ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md`

## Closure Note

Closed after the audit classified the remaining AArch64 codegen layout residue
and created scoped follow-up ideas. No implementation files changed. The audit
did not reopen the earlier shared-authority routes because ideas 69-78 either
consumed the needed prepared facts or recorded evidence that no new shared
authority is required for the reviewed paths.

The next work should choose one of the new open ideas instead of attempting a
single monolithic codegen shrink. The safest first implementation target is the
local-helper duplication tail, because it is narrow, AArch64-local, and already
called out by idea 74's closure note.

