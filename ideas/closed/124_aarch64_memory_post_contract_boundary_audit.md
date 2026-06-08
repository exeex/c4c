# 124 AArch64 Memory Post-Contract Boundary Audit

## Goal

Audit `src/backend/mir/aarch64/codegen/memory.cpp` after the recent prepared,
publication, calls, dispatch, and comparison contract work, then produce
focused follow-up ideas for any remaining memory-side boundary gaps.

This idea is analysis-only. It should not directly edit implementation files.

## Why This Exists

`memory.cpp` remains the second-largest AArch64 codegen owner at roughly 5.2k
lines. Earlier memory work deliberately folded target-local store-source
emission residue back into the memory owner after moving shared store-source
facts forward. Other closed routes also clarified prepared memory authority,
frame-slot address materialization, store retargeting, prepared dump
visibility, call argument/result publication, and current-block publication
contracts.

The remaining question is not "how do we shrink `memory.cpp` by line count?"
It is whether `memory.cpp` still mixes shared prepared/BIR authority with
AArch64 memory emission. If shared authority remains, it should move forward
before any mechanical owner split. If the boundary is clean, follow-up work
should focus on local owner clarity or explicitly record that no new idea is
needed.

## In Scope

Audit the current memory owner using these standards:

1. Prepared memory record and identity validation
   - Decide whether `memory.cpp` still owns target-neutral consistency or
     identity decisions that should be shared prepared queries.
   - Keep AArch64 operand legality, addressing mode selection, and diagnostic
     spelling target-local.

2. Store-source and store-global publication consumption
   - Check whether store-source/store-global paths are only consuming prepared
     publication facts or still maintaining target-neutral publication state.
   - Include pending store-global stack-value publication consumers such as
     `lower_pending_store_global_stack_value_publications`.

3. Frame-slot and pointer-base materialization residue
   - Verify that frame-slot, pointer-base-plus-offset, fixed-slot, and
     prepared-address paths consume closed-route prepared authority instead of
     value-name recovery or local fallback selection.

4. Variadic `va_list` field memory handling
   - Separate shared variadic payload/home facts from AArch64 ABI-sensitive
     load/store shape, field address, register, and memory operand emission.

5. Physical split readiness
   - If the above boundaries are clean, decide whether local memory owner
     surfaces should be split for clarity.
   - Treat physical splitting as mechanical clarity, not semantic progress.

## Relationship To Prior Work

Use these closed ideas as required context:

- idea 34: store-source publication planning
- idea 39 / 39a: memory store-source fold-back after semantic residue cleanup
- idea 50: memory prepared authority repair
- idea 70: memory prepared address authority cleanup
- idea 86: memory owner subresponsibility audit
- idea 88: memory frame-slot address materialization owner
- idea 89: memory store retargeting owner
- idea 111: store-source publication dump visibility
- idea 114: prepared outgoing stack argument area contract
- ideas 116 / 117: shared current-block and producer publication contracts
- ideas 122 / 123: shared call argument/result producer and late-publication
  contracts

Do not duplicate those routes. If one of the five audit standards is already
fully covered, the closure note should explicitly say "no new idea" for that
standard and cite the closed evidence.

## Expected Outputs

The closure note should include:

- a classification table for the five audit standards;
- a function/helper cluster map for the current large `memory.cpp` regions;
- explicit "no new idea" entries for standards already covered by closed work;
- follow-up `ideas/open/` files only for concrete unresolved gaps.

Good follow-up idea shapes include:

- a shared prepared query/fact idea if memory still owns target-neutral memory
  identity, publication, or materialization authority;
- a contract-visibility idea if cleanup is blocked by missing dump or route
  evidence;
- a local AArch64 memory owner extraction idea if the boundary is clean and the
  split improves private owner clarity;
- a no-op closure note when current evidence proves `memory.cpp` is large but
  correctly target-local.

## Out Of Scope

- Direct implementation edits in `memory.cpp`, `memory.hpp`, tests, or build
  metadata.
- Reopening already closed store-source, frame-slot, store-retargeting,
  prepared-address, or publication-contract routes without new evidence.
- Moving AArch64 addressing legality, scratch/base register policy, load/store
  opcode choice, ABI-specific `va_list` field layout, or machine-record
  emission into shared BIR/prealloc.
- Treating line-count reduction as acceptance.
- Starting x86 or RISC-V codegen implementation.

## Acceptance And Completion Criteria

- The audit creates no implementation, test, or build metadata changes.
- Each generated follow-up idea names:
  - owner boundary;
  - likely files;
  - proof route;
  - testcase-overfit reject signals.
- The closure note distinguishes shared prepared/BIR authority from AArch64
  memory emission for each of the five standards.
- No follow-up duplicates ideas 34, 39, 39a, 50, 70, 86, 88, 89, 111, 114,
  116, 117, 122, or 123.

## Reviewer Reject Signals

- The route proposes one monolithic `memory.cpp` shrink implementation.
- It treats a large helper cluster as wrong without identifying whether the
  responsibility is shared authority, target emission, or review visibility.
- It creates vague follow-up ideas without concrete files or proof routes.
- It reopens closed memory/store-source/frame-slot/retargeting work without
  new evidence.
- It proposes moving AArch64-specific addressing, opcode, scratch, ABI, or
  machine-record emission policy into shared code.
- It claims progress from physical file movement while the same mixed authority
  remains hidden behind new helper names.

## Closure Evidence

Closed as an analysis-only boundary audit. No implementation, test, or build
metadata changes were required for this closure.

### Five-Standard Classification

| Audit standard | Closure result | Evidence summary |
| --- | --- | --- |
| Prepared memory record and identity validation | No new idea | `make_memory_record_from_prepared_access`, memory fact validation, base identity validation, load/store identity application, and related diagnostic wrappers consume prepared memory/address/value facts and keep AArch64 record construction, operand legality, diagnostics, and machine wrapping local. Closed ideas 70 and 86 already cover this split. |
| Store-source and store-global publication consumption | Follow-up idea created | Store-local publication paths now consume prepared store-source, recovered narrow-source, direct-global select-chain, byval, and current-block publication facts. One remaining shared-authority gap was found in `lower_pending_store_global_stack_value_publications`, which still rediscovered the pending store-global source producer by scanning earlier BIR instructions and matching result name/type after `prepare::plan_pending_prepared_store_global_publications`. |
| Frame-slot and pointer-base materialization residue | No new idea | Frame-slot, pointer-base, fixed-slot, prepared-address, stack-homed pointer, and pointer-base-plus-offset paths consume prepared value-home/storage/frame-address/materialization facts while AArch64 keeps scratch choice, register views, offset legality, address spelling, retargeting, and assembler emission local. Closed ideas 70, 88, and 89 cover this boundary. |
| Variadic `va_list` field memory handling | No new idea | `find_va_list_field_address`, `make_va_list_field_memory_operand`, va-list field load/store record builders, cursor-update producer detection, and cursor-update machine instruction emission remain ABI-specific AArch64 memory handling. No duplicate frame-slot, value-home, or store-source authority was found in this cluster, matching idea 86's target-local classification. |
| Physical split readiness | Deferred local-clarity only | `memory.cpp` remains physically large, but the audit found mostly prepared-fact consumers plus AArch64 emission/recording helpers. Physical splitting is not semantic progress and is deferred until the shared pending store-global publication producer gap is resolved or explicitly scoped out. |

### Current `memory.cpp` Cluster Map

- Basic AArch64 memory spelling and public surface: register-indirect address
  spelling, scalar load/store mnemonics, dispatch publication scalar sizing,
  and fixed-formal store mnemonic selection.
- Diagnostics, machine-record wrapping, byte-immediate stores, and scalar store
  offset legality: prepared name helpers, error wrappers, diagnostic appending,
  BIR machine instruction wrapping, store-width checks, and scalar store
  emission.
- Prepared storage and before-return retargeting: storage-plan lookup,
  frame-pointer base policy, memory return ABI register lookup, and result
  retargeting.
- Prepared memory identity and operand construction: indexed value-home lookup,
  memory fact validators, base identity validation, load/store identity
  application, and prepared memory record construction.
- Register and value-home decoding plus stack-source scratch selection:
  register view helpers, value-home register operands, pointer base resolution,
  prepared load/store storage decoding, and stack-source publication scratch
  selection.
- Machine node metadata and selection status: memory opcode/effect metadata,
  immediate-store support, base support, symbol identity checks, and fail-closed
  memory selection status.
- Prepared-access queries, names, addresses, diagnostics, result recording, and
  result retargeting: prepared access matching, memory address/name helpers,
  transport diagnostics, memory result recording, prepared-home retargeting,
  memory operand creation, and final memory instruction creation.
- Prepared scalar load/store record builders for local/global memory:
  load-local, store-local, load-global, store-global, and shared load/store
  memory instruction record builders.
- Variadic `va_list` field memory handling: field suffix parsing, field
  address lookup, field memory operands, field load/store records,
  cursor-update producer detection, and cursor-update emission.
- Main scalar memory lowering: prepared addressing/storage consumption,
  va-list special handling, stack-layout and return-ABI retargeting,
  frame-pointer policy, byte-immediate special emission, and final
  machine-record construction.
- F128/i128 memory-backed transport lowering: prepared carrier/memory operand
  consumers for transport records.
- Pointer-value load publication: prepared pointer value loading, byval and
  variadic exclusions, and stack-homed pointer value load publication.
- Store-source publication planning glue: prepared recovered narrow-source and
  source-producer consumers, direct/global load-local helpers, scalar
  conversion emission, destination object helpers, store-local source
  publication planning, pointer writeback planning, and pointer-base-plus-offset
  publication planning.
- Fixed-formal and store-local publication emission: fixed-formal publication
  planning/emission and store-local value publication emission around prepared
  producer, recovered-source, direct-global, current-block, scratch, and
  stack-home facts.
- Stack-homed pointer writeback and pointer-base-plus-offset materialization:
  prepared pointer-store writeback consumption, global symbol address emission,
  prepared pointer-base-plus-offset materialization lookup, and AArch64 address
  materialization.
- Store-global publication and pending stack publication: prepared
  store-global addressing, stack publication home lookup, pending stack-value
  publication lowering, published stack-value publication emission, future
  coverage checks, and store-global publication plan consumption.

### Follow-Up Decisions

- Created
  `ideas/open/125_prepared_pending_store_global_publication_producer_contract.md`
  for the remaining shared prepared/prealloc producer contract gap in pending
  store-global stack-value publication.
- No new idea for prepared memory identity.
- No new idea for frame-slot or pointer-base materialization.
- No new idea for variadic `va_list` field memory handling.
- No physical split idea was created. Any AArch64 memory-private extraction is
  deferred as local-clarity-only work until idea 125 is resolved or the shared
  pending store-global gap is explicitly scoped out.
