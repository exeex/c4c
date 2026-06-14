Status: Active
Source Idea Path: ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Build the Producer/Source Fail-Closed Matrix

# Current Packet

## Just Finished

Completed Step 5, "Build the Producer/Source Fail-Closed Matrix", as an
analysis-only matrix for idea 252.

Selected relation carried forward:

- Same-edge CFG publication source-producer identity. The prepared
  `PreparedEdgePublication` answer populated from
  `PreparedFunctionLookups::edge_publication_source_producers` must agree with
  the Route 5/BIR record for the same predecessor, successor, destination,
  source value, producer kind, producer block, and producer instruction index.
- `LoadLocal` additionally requires Route 5 `memory_source` plus Route 3
  source-memory identity agreement with prepared source-memory fields.
- `available`, `memory_source`, `no_match`, `no_source`,
  `missing_source_producer`, and related Route 5 rows are agreement/rejection
  evidence only. Prepared lookup/status/output rows remain compatibility
  surfaces until a later adapter can reject disagreement fail-closed.

Producer/source fail-closed matrix:

| Case | Prepared compatibility surface that must remain observable | Required Route 5/BIR behavior for any later adapter | Scope | Existing evidence |
| --- | --- | --- | --- | --- |
| Duplicate Route 5 edge records | Keep public prepared helper and printer rows, including `source_producer=...`, `source_producer_block=...`, `source_producer_inst=...`, and prepared lookup helper names. Do not rename or hide `edge_publication_source_producers`. | Duplicate records for the same predecessor/successor/destination/source relation must reject authority transfer. A diagnostic duplicate/no-match row may be exposed, but the adapter must not choose the first record, merge records, or fall back to old prepared authority while claiming semantic migration. | Common. | RISC-V test evidence clones a Route 5 edge record and requires duplicate/mismatch/absence diagnostic visibility in `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1652`. Route index status names include fail-closed duplicate/no-match patterns in `src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp`. |
| Conflicting producer record for the same source value | Keep prepared producer kind strings `unknown`, `immediate`, `load_local`, `load_global`, `cast`, `binary`, and `select_materialization`, plus prepared scalar helper/oracle names. | If Route 5/BIR points at a different producer kind, producer block, instruction index, or producer instruction than the prepared answer, the relation must reject as disagreement. A named-case shortcut that only checks value name or output text is invalid. | Common. | Agreement rule from Step 2 requires producer kind, block label/id, instruction index, and instruction pointer identity. RISC-V `route5_edge_source_agrees_with_prepared_publication(...)` compares kind and requires a non-null producer instruction/index for named sources in `src/backend/mir/riscv/codegen/emit.cpp:394`. Prepared helpers validate pointer/result identity in `src/backend/prealloc/prepared_lookups.cpp:773` and `src/backend/prealloc/prepared_lookups.cpp:844`. |
| Destination/source mismatch | Keep prepared publication lookup status, source/destination value rows, source home rows, and Route 5 diagnostic strings such as `no_match`. | Destination value name/type, source value kind/name/type, predecessor, and successor must all agree. Type/name/edge mismatches must produce rejection evidence such as `no_match`, never authority transfer through matching assembly or matching helper status text. | Common. | `route5_find_cfg_edge_publication(...)` reports `no_match` for destination type mismatch and `no_source` for predecessor absence in `src/backend/bir/bir.cpp:2077`. RISC-V tests assert type mismatch reports `no_match` and clears `route5_edge_source_agrees` while preserving prepared fallback output in `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1652`. |
| Missing/absent Route 5 or BIR source | Keep prepared statuses `missing_publication`, `missing_source_producer`, `missing_source_memory_access`, `incomplete_source_memory_access`, and the public prepared lookup surface. | Missing index, missing predecessor/successor/destination, absent source, missing source value, or missing source producer must reject semantic agreement. The adapter must not silently use prepared-only lookup and call that Route 5/BIR agreement. | Common. | Route 5 statuses include `missing_predecessor`, `missing_successor`, `missing_destination`, `missing_publication`, `missing_source_value`, `missing_source_producer`, `no_source`, and memory-source missing rows in `src/backend/bir/bir.hpp:1650`. x86 is blocked because no x86 consumer joins prepared publication to `BirCfgEdgePublicationSourceIdentity` or `Route5CfgEdgePublicationRecord`. |
| Unsupported prepared publication or homes | Preserve x86/RISC-V move-intent statuses `MissingSharedLookups`, `MissingPublication`, `UnsupportedPublication`, `UnsupportedSourceHome`, `UnsupportedDestinationHome`, and `Available`. | Unsupported prepared publication, source home, destination home, or non-move publication must remain a target compatibility rejection. Route 5 agreement cannot launder an unsupported prepared output row into semantic readiness. | Target-specific. | x86 exposes these statuses in `src/backend/mir/x86/prepared/prepared.hpp:154` and consumes prepared move/home rows in `src/backend/mir/x86/prepared/dispatch.cpp:55`. RISC-V returns the same status family from `src/backend/mir/riscv/codegen/emit.cpp:501`; tests keep missing lookup, missing publication, malformed homes, unsupported homes, and non-move rows fail-closed in `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1797`. |
| Prepared-only authority | Keep `PreparedFunctionLookups::edge_publication_source_producers`, `PreparedEdgePublication::source_producer_*`, `find_indexed_prepared_edge_publication_source_producer(...)`, and prepared helper/oracle tests observable. | Prepared-only evidence may remain compatibility authority, but it must not count as semantic Route 5/BIR agreement. A later adapter must require explicit agreeing Route 5/BIR identity or report blocked/non-applicable. | Common. | Step 1 found prepared/AArch64 consumers keep the public helper surface stable. x86 Step 3 remains blocked: x86 consumes prepared edge-publication move/home status but no Route 5/BIR source-producer identity. RISC-V normal emission still calls the overload without a Route 5 record in `src/backend/mir/riscv/codegen/emit.cpp:733`. |
| Fallback on non-agreeing route facts | Preserve fallback names, instruction text, and target output rows such as x86 `mov`, RISC-V `mv a1, a0`, and RISC-V `lw a1, 12(s2)` as target/output compatibility only. | A later authority-transfer adapter must reject non-agreeing Route 5/Route 3 facts. It may not preserve prepared fallback output and simultaneously claim semantic agreement. Existing fallback output is acceptable only when classified as non-authoritative compatibility behavior. | Target-specific. | RISC-V tests prove non-agreeing Route 5 type mismatch and Route 3 memory mismatch/incomplete rows preserve prepared fallback output while clearing agreement booleans in `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1674`, `:1749`, and `:1768`. x86 `mov` output remains prepared move/home policy in `src/backend/mir/x86/module/module.cpp:2523`. |
| `LoadLocal` memory-source mismatch or incompleteness | Keep source-memory status strings `unavailable`, `available`, `missing_prepared_memory_access`, and `incomplete_prepared_memory_access`, plus Route 3 diagnostic rows. | `LoadLocal` agreement requires Route 5 `memory_source`, `source_memory_identity_available`, and Route 3 memory access matching prepared result value, base identity, address space, volatility, byte offset, size, and alignment. Any mismatch or incomplete Route 3 row must reject. | Common for `LoadLocal`; not applicable to non-memory producer kinds. | RISC-V comparison function enforces Route 3 fields in `src/backend/mir/riscv/codegen/emit.cpp:366`; agreeing dynamic memory-source evidence is tested at `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1705`, with mismatch/incomplete rejection at `:1749` and `:1768`. |
| Immediate source producer | Preserve immediate producer kind and prepared publication source value rows. | Immediate sources may agree only through immediate value/type and `Immediate` producer kind. Any adapter must not require a source producer instruction for immediates and must not treat named-source missing-producer behavior as equivalent to immediate agreement. | Common. | Step 2 agreement rule separates immediate sources from named sources. RISC-V `route5_edge_source_agrees_with_prepared_publication(...)` checks immediate integer constant/type without requiring an instruction in `src/backend/mir/riscv/codegen/emit.cpp:394`. |
| Policy-sensitive target rows | Preserve target policy rows: register/home/storage policy, stack-slot width/offset policy, pointer-base materialization, scratch/carrier choices, branch/parallel-copy placement, and final assembly spelling. | Route 5/BIR source-producer agreement cannot be inferred from policy acceptance or final output. Policy failures remain unsupported/status rows; policy success remains target emission behavior, not producer/source authority. | Target-specific. | x86 i32-only `prepared_edge_publication_move_has_i32_operands(...)` and final `mov` output live under `src/backend/mir/x86/module/module.cpp:2523`. RISC-V source/destination homes, stack-source, pointer-base, and instruction text are prepared-backed policy in `src/backend/mir/riscv/codegen/emit.cpp:528` through `:699`. |

Reviewer rejection rules derived from the matrix:

- Reject expectation weakening: do not downgrade supported-path tests to
  unsupported, delete diagnostic rows, or rewrite expected output to hide
  disagreement.
- Reject named-case shortcuts: checking one fixture name, one instruction
  spelling, one helper name, or one fallback string is not producer/source
  agreement.
- Reject helper/status rewrites: renaming, privatizing, wrapping, bypassing,
  or deleting public prepared helpers/status strings does not prove migration.
- Reject old-failure retention as progress: preserving prepared fallback on a
  mismatch remains compatibility behavior unless the route is explicitly
  classified non-authoritative or blocked.
- Reject x86 readiness claims until x86 has a direct or indirect consumer that
  joins prepared publication rows to the same Route 5/BIR source-producer
  identity and rejects disagreement.
- Reject RISC-V authority-transfer claims until non-agreeing Route 5/Route 3
  rows fail closed instead of merely clearing diagnostic agreement booleans
  while preserving prepared fallback/output.

## Suggested Next

Execute Step 6, "Decide Producer-Adapter Readiness and Close or Split", as an
analysis-only closure packet. Summarize that x86 is blocked, RISC-V has
diagnostic agreement but not fail-closed authority transfer, and this matrix
requires any implementation follow-up to be a separate narrow idea with
explicit duplicate/conflict/mismatch/missing/prepared-only/fallback/policy
rejection tests before adapter readiness can be claimed.

## Watchouts

- Keep source idea 252 unchanged unless durable intent truly changes.
- Do not implement a BIR producer index or adapter in this blocker-map plan.
- Do not delete, privatize, wrap, rename, or bypass public prepared
  `edge_publication_source_producers` helpers.
- Step 3 left x86 blocked, not non-applicable.
- Step 4 left RISC-V diagnostic agreement evidence, not fail-closed authority
  transfer.
- Treat target output, fallback names, helper/status strings, and home/storage
  policy as compatibility or target-policy surfaces, not semantic agreement.
- Step 6 should not close this as adapter-ready unless it preserves the
  blockers above or splits a new implementation idea with fail-closed proof
  requirements.

## Proof

Analysis-only packet. No build or test proof required by supervisor.
Local validation: `git diff --check -- todo.md` passed.
