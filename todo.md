Status: Active
Source Idea Path: ideas/open/59_aarch64_dispatch_family_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish Audit Evidence Baseline

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit evidence baseline for idea 59.

Audited dispatch-family files:

- `dispatch.hpp` / `dispatch.cpp`: public `make_block_lowering_context` and
  `dispatch_prepared_block`; local block traversal, instruction-family
  classification, diagnostics, before-return publication bookkeeping, address
  materialization routing, and branch-fusion hook wiring.
- `dispatch_edge_copies.hpp` / `dispatch_edge_copies.cpp`: edge-copy public
  surface for edge producer contexts, prepared edge source/register/memory
  materialization, predecessor join publication, block-entry copy filtering,
  predecessor select parallel copies, select-chain materialization, and direct
  global select-chain call-argument materialization.
- `dispatch_lookup.hpp` / `dispatch_lookup.cpp`: public prepared-name,
  prepared-value-home, emitted-scalar, same-block scalar producer, and
  same-block load-local availability lookup helpers.
- `dispatch_producers.hpp` / `dispatch_producers.cpp`: public same-block
  binary/select/named producer helpers, prepared select producer bridge,
  integer constant evaluation, direct-global select-chain scan, load-global
  target/symbol helpers, and current-block join parallel-copy cache helpers.
- `dispatch_publication.hpp` / `dispatch_publication.cpp`: public helper
  inventory includes register alias/view helpers, frame/slot/address spelling,
  scalar load/store mnemonics, prepared local load offset, va-list/local-slot
  address helpers, block-entry/current-publication recording and lookup,
  missing branch/fused-compare publication, memory/address result recording,
  retargeting helpers, fixed-formal store-local publication, clobber checks,
  publication register-read checks, and BIR result-value helpers.
- `dispatch_value_materialization.hpp` /
  `dispatch_value_materialization.cpp`: public
  `emit_value_publication_to_register`; local helpers for prepared same-block
  scalar producers, prepared global-load emission, power-of-two evaluation,
  scratch-write hazard checking, load-local/global/select/local-slot
  materialization, and recursive operand materialization.

Comparison surfaces recorded:

- Reference ARM layout:
  `ref/claudes-c-compiler/src/backend/arm/codegen/mod.rs` and sibling
  `emit`, `alu`, `memory`, `globals`, `calls`, `comparison`, `cast_ops`,
  `float_ops`, `f128`, `i128_ops`, `intrinsics`, `inline_asm`, `returns`,
  `prologue`, `variadic`, `atomics`, `asm_emitter`, and `peephole` shards.
- Live AArch64 route note:
  `src/backend/mir/aarch64/codegen/README.md` identifies `dispatch.cpp` as
  prepared block traversal/routing and `dispatch_*` as narrower lowering
  owners behind dispatch.
- Shared backend authority surfaces:
  `src/backend/prealloc/prepared_lookups.*`,
  `src/backend/prealloc/value_locations.hpp`,
  `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/regalloc/*`,
  `src/backend/prealloc/formal_publications.*`,
  `src/backend/prealloc/addressing.hpp`,
  `src/backend/mir/query.*`, and `src/backend/bir/lir_to_bir/*`.

Public helper/caller inventory:

- `dispatch_prepared_block` and `make_block_lowering_context` are called from
  `traversal.cpp`; `dispatch_prepared_block` calls the edge-copy and
  publication public surfaces during block-entry, instruction, memory,
  scalar, terminator, and post-block edge-copy phases.
- Edge-copy public helpers are mostly dispatch-local except
  `emit_select_chain_value_to_register`, also called by `alu.cpp` and
  `dispatch_value_materialization.cpp`, and
  `materialize_direct_global_select_chain_call_argument`, called by
  `calls.cpp`.
- Lookup helpers are cross-family utilities: used by dispatch, producers,
  edge copies, publication, value materialization, globals, memory,
  comparison, cast ops, calls, FP value materialization, and ALU paths.
- Producer helpers are used by dispatch branch-fusion hooks, edge copies,
  FP value materialization, globals, value materialization, comparison, and
  the current-block join parallel-copy cache in dispatch.
- Publication helpers are used by dispatch plus globals, edge copies,
  value materialization, FP value materialization, cast ops, comparison,
  memory, ALU, and calls.
- Value materialization public entry is called by dispatch and related
  publication/select materialization paths through edge-copy helpers.

Local helper groups inventoried:

- Routing/diagnostic group: `classify_instruction`, block diagnostics,
  unsupported terminator/instruction diagnostics, BIR block lookup, and
  block-context lookup.
- Dispatch orchestration group: before-return publication tracking,
  address-materialization routing, scalar/store-local materialization routing,
  branch-fusion hook construction, and prepared memory-index retry logic.
- Edge prepared-consumer group: prepared edge-publication source matching,
  producer block/source context recovery from prepared facts, prepared memory
  access matching, source home/register/memory extraction, and va-list carrier
  emission.
- Edge local-fallback/hazard group: `unique_branch_predecessor_context`,
  `find_edge_named_producer`, recursive edge register-read checks, scratch and
  result-register selection, and select-chain label/materialization emission.
- Lookup group: prepared-name/value-id/value-home bridges plus same-block
  scalar/load-local availability helpers.
- Producer group: wrappers over `mir::query` same-block scans, prepared
  same-block select bridge, direct-global select-chain dependency traversal,
  load-global symbol resolution, and current-block join parallel-copy cache.
- Publication target-spelling group: frame/slot/register address spelling,
  register view selection, scalar load/store mnemonics, register aliasing, and
  fixed-frame-pointer selection.
- Publication prepared-consumer group: prepared local load offsets, prepared
  value-home/block-entry publication checks, prepared source producer lookup,
  branch/fused-compare publication through prepared branch/scalar facts,
  before-return FPR ABI lookup, fixed-formal store-source planning, and
  retargeting through prepared homes.
- Value-materialization group: prepared same-block scalar producer lookup,
  prepared global-load emission, value-home publication, load-local prepared
  memory/recovered source consumption, global-load prepared access
  consumption, select-chain materialization, local-slot address publication,
  and scratch/read-write hazard checks.

Residual candidates needing closer Step 2 classification:

- Raw producer scans still exposed as public or local authority candidates:
  `dispatch_lookup::{find_same_block_scalar_producer,
  has_same_block_load_local_producer}`,
  `dispatch_producers::{find_same_block_binary_producer,
  find_same_block_select_producer, find_same_block_named_producer,
  find_same_block_result_index, same_block_result_depends_on_value,
  evaluate_same_block_integer_constant}`, and edge
  `find_edge_named_producer`. Some are wrappers over shared `mir::query` and
  may be valid convenience surfaces, but Step 2 must distinguish convenience
  from semantic authority.
- Local fallback authority candidates:
  edge `find_edge_named_producer` still walks successor/edge/predecessor
  contexts when prepared publication is absent; edge/value/publication
  recursive register-read checks can fall back from prepared producer facts to
  same-block named producer records; `local_slot_address_frame_offset` remains
  a null implementation while local/aggregate address helpers consume prepared
  frame-address offsets.
- Duplicate semantic recovery candidates:
  select-chain/direct-global paths span `dispatch_edge_copies.cpp`,
  `dispatch_producers.cpp`, `dispatch_value_materialization.cpp`, `alu.cpp`,
  and `calls.cpp`; global-load symbol/policy helpers are split between
  producer lookup and prepared global-load emission; current-block join
  parallel-copy caches rebuild move/source relationships locally even though
  they consult prepared value homes and move bundles.
- Keep-local evidence to preserve while classifying: target instruction
  spelling, register view/name selection, scratch/clobber avoidance, frame
  address rendering, branch/select label emission, diagnostics, and final
  machine-instruction sequencing.

## Suggested Next

Proceed to `plan.md` Step 2 by classifying each audited file/helper group as
`fold-back`, `move-forward`, `keep-local`, `split-owner`, or
`needs-more-evidence`, using the residual candidates above as the first
classification targets.

## Watchouts

- This plan is audit-only until follow-up ideas are activated.
- Do not edit implementation files as part of the classification work.
- Keep follow-up ideas concrete: name owned files, ownership class, and reject
  signals.
- Ideas 47, 48, and 49 closed after routing edge-copy, publication, and value
  materialization fallback families through prepared/shared authority. Treat
  new same-block scans, predecessor-depth scans, raw move-bundle scans, or
  string/lane frame-offset recovery as suspect unless Step 2 proves they are
  target-local convenience rather than semantic authority.
- Idea 58 closed after the remaining prepared-authority regression set was
  repaired; this audit should not reopen those failures or claim capability
  progress from classification-only notes.

## Proof

Audit-only packet. No build or test required; no `test_after.log` update made.
Evidence gathered with `c4c-clang-tool-ccdb` function-signature/list-symbol
queries for all six dispatch-family `.cpp` files, header signature reads for
all six dispatch-family `.hpp` files, targeted caller/reference searches, and
source-idea plus closure-note review for ideas 47, 48, 49, and 58.
