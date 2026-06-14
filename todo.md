Status: Active
Source Idea Path: ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Recheck Riscv Evidence Against the Same Fact

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Recheck Riscv Evidence Against the Same Fact",
as an analysis-only RISC-V classification for the selected Route 3 `LoadLocal`
result/source-memory identity fact.

Result: RISC-V has narrow evidence for the selected identity, but only the
agreement flags and source-memory fields are semantic evidence. The generated
`lw` text, register choices, source-home policy, fallback names, and status
strings remain prepared compatibility or target policy.

The selected fact remains unchanged from Step 2: Route 3 `LoadLocal`
result/source-memory identity agreement for a prepared dynamic source-memory
row. A Route 3 `Route3MemoryAccessRecord` for a same-function, same-block
`LoadLocal` source must identify the same memory access as
`PreparedEdgePublication::source_memory_access` when both sides describe the
same loaded result value and source-memory fields.

True RISC-V identity evidence:

- `src/backend/mir/riscv/codegen/emit.cpp:366`
  `route3_source_memory_agrees_with_prepared_publication(...)` compares the
  prepared publication against a Route 3 memory record. It requires a prepared
  `LoadLocal` source producer, `Available` prepared source-memory status,
  non-null `source_memory_access`, Route 3 node kind `LoadLocal`, matching
  result value kind/type/name, matching address space, volatility, byte offset,
  size, alignment, and matching base identity.
- `src/backend/mir/riscv/codegen/emit.cpp:394`
  `route5_edge_source_agrees_with_prepared_publication(...)` treats Route 5
  `MemorySource` rows for prepared `LoadLocal` producers as agreeing only when
  `source_memory_identity_available` is true and
  `route3_source_memory_agrees_with_prepared_publication(...)` succeeds.
- `src/backend/mir/riscv/codegen/emit.cpp:486`
  `attach_route5_edge_agreement(...)` records
  `intent.route3_source_memory_agrees` from the Route 3/prepared comparison
  when the Route 5 row carries source-memory identity. This is the RISC-V
  diagnostic bit that proves or rejects the selected fact without taking over
  output authority.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1699`
  through `:1735` builds a Route 3 `LoadLocal` memory access index and checks
  the record as `load_local`, `pointer_value`, `%base`, byte offset `12`, and
  size `4`; it then checks a Route 5 `MemorySource` row whose
  `source_memory_access` points at the same Route 3 instruction and agrees with
  the prepared publication's source-memory byte offset and size.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1736`
  through `:1745` calls `consume_edge_publication_move_intent(...)` with the
  agreeing Route 5/Route 3 row and expects
  `intent.route3_source_memory_agrees` to be true.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1748`
  through `:1762` mutates the Route 3 byte offset to `16` and expects
  `intent.route3_source_memory_agrees` to be false while RISC-V output remains
  prepared-backed.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1766`
  through `:1781` clears Route 3 source-memory identity and expects
  `intent.route3_source_memory_agrees` to be false while RISC-V output remains
  prepared-backed.

Prepared publication evidence for the same fact:

- `src/backend/prealloc/prepared_lookups.cpp:443`
  `copy_source_memory_access_fact(...)` copies prepared source-memory base,
  frame/global/pointer identity, byte offset, size, alignment, address space,
  volatility, and base-plus-offset/materialization flags into
  `PreparedEdgePublication`.
- `src/backend/prealloc/prepared_lookups.cpp:461`
  `apply_source_memory_access_fact(...)` publishes source-memory facts only for
  `PreparedEdgePublicationSourceProducerKind::LoadLocal`, reports
  `MissingPreparedMemoryAccess` when the prepared access is absent, reports
  `IncompletePreparedMemoryAccess` when result/base/size/alignment are not
  complete, and reports `Available` only after copying a complete prepared
  access.
- `src/backend/prealloc/prepared_lookups.cpp:1443` calls
  `apply_source_memory_access_fact(...)` while building prepared edge
  publications, so the RISC-V consumer sees prepared source-memory rows through
  shared lookup authority.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1377`
  through `:1403` checks the dynamic `LoadLocal` publication itself:
  source producer `LoadLocal`, source-memory status `Available`, non-null
  source-memory access, pointer-value base, `%base`, byte offset `12`, size
  `4`, alignment `4`, and base-plus-offset available without address
  materialization.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1465`
  through `:1508` distinguishes missing and incomplete prepared source-memory
  rows and checks that RISC-V rejects them as `UnsupportedSourceHome` without
  inventing source-memory output.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:1555`
  through `:1585` keeps complete but unsupported source-memory shapes, such as
  materialization-required and non-i32 dynamic loads, fail-closed.

AST-backed confirmation:

- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp
  consume_edge_publication_move_intent build-x86/compile_commands.json`
  reported the public overload delegating into
  `RiscvEdgePublicationMoveAdapter::consume_prepared_backed_move_intent`.
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp
  route3_source_memory_agrees_with_prepared_publication
  build-x86/compile_commands.json` reported
  `route3_base_kind_agrees_with_prepared_source_memory(...)` as the local
  semantic helper.
- `c4c-clang-tool-ccdb function-callers
  /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp
  route3_source_memory_agrees_with_prepared_publication
  build-x86/compile_commands.json` reported
  `route5_edge_source_agrees_with_prepared_publication(...)` as the caller,
  matching the Route 5 diagnostic row path used by the test.
- `c4c-clang-tool-ccdb list-symbols
  /workspaces/c4c/tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp
  build-x86/compile_commands.json` identified the relevant helper/test symbols:
  `make_load_local_dynamic_stack_source`,
  `load_local_source_access`,
  `check_dynamic_stack_source_uses_shared_source_memory_access`, and
  `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback`.

Non-semantic RISC-V rows that must remain compatibility-owned or target-owned:

- Exact instruction text such as `lw a1, 12(s2)`, `lw a1, 16(sp)`, and
  multi-instruction large-offset forms.
- RISC-V register choices, including destination GPR selection, base register
  selection such as `s2`, scratch registers such as `t6`/`t0`, and any
  float-register rejection behavior.
- Source-home policy: `StackSlot`, `PointerBasePlusOffset`, immediate,
  aggregate stack source, dynamic source-home support, concrete stack offsets,
  large-offset legality, and unsupported source/destination home decisions.
- Prepared fallback and status strings such as `MissingSharedLookups`,
  `MissingPublication`, `UnsupportedSourceHome`,
  `UnsupportedDestinationHome`, `UnsupportedPublication`,
  `MissingPreparedMemoryAccess`, `IncompletePreparedMemoryAccess`,
  `available`, `memory_source`, `no_match`, and `no_source`.
- Addressing/storage-sensitive output: base-plus-offset legality,
  address-materialization requirement, non-i32 dynamic loads, volatile or
  non-default address-space rows, stack/source size and alignment policy, and
  final assembly spelling.
- Helper/oracle names and prepared publication lookup behavior, including
  `consume_edge_publication_move_intent(...)`,
  `find_unique_indexed_prepared_edge_publication(...)`,
  `route5_cfg_edge_publication_record(...)`, and
  `route3_find_memory_access_record(...)`.

Classification:

- RISC-V is proven for the selected identity as a diagnostic agreement row:
  agreeing Route 3 `LoadLocal` source-memory facts make
  `route3_source_memory_agrees` true, mismatched or incomplete Route 3
  source-memory facts make it false, and prepared output remains stable.
- RISC-V is not proof that Route 3 owns output emission. The current consumer
  intentionally preserves prepared-backed emission and fallback behavior even
  when Route 3 diagnostics are absent, mismatched, or incomplete.
- No RISC-V blocker was found for recording the selected identity evidence,
  but adapter readiness still depends on the Step 5 fail-closed matrix and the
  earlier Step 3 x86 blocker.

## Suggested Next

Execute Step 5 by building the fail-closed proof matrix for the selected Route
3 `LoadLocal` result/source-memory identity. The next packet should enumerate
missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
fallback, and policy-sensitive rows, while keeping prepared lookup/status
behavior and target output policy observable.

## Watchouts

- Step 3 left x86 blocked for the same selected fact, so Step 5 should not
  treat RISC-V agreement as cross-target adapter readiness.
- Count only `route3_source_memory_agrees`, Route 3 memory record fields, and
  prepared source-memory fields as semantic evidence. Do not count `lw` text,
  base/destination registers, source-home status, fallback strings, or helper
  names as semantic Route 3 ownership.
- RISC-V preserves prepared fallback on mismatched or incomplete Route 3
  diagnostics; that is intentional compatibility behavior, not a failure.
- Keep the selected fact narrow. Do not broaden into generic memory parity,
  final instruction formatting, or an implementation plan.

## Proof

No build/test proof required by the delegated packet. Analysis-only validation
used targeted `rg`, focused `sed`, and the `c4c-clang-tool-ccdb` commands
recorded above. No `test_after.log` produced because proof was explicitly not
required.
