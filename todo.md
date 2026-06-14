Status: Active
Source Idea Path: ideas/open/251_phase_f3_route45_edge_publication_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Recheck Riscv Evidence Against the Same Fact

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Recheck Riscv Evidence Against the Same Fact",
as an analysis-only evidence packet.

Classification: riscv has diagnostic evidence for the selected Route 5
CFG-edge publication source identity, but prepared lookup/status and target
emission remain authoritative. This is evidence for agreement rows, not an
adapter-ready semantic authority transfer.

Concrete riscv evidence found:

- `src/backend/mir/riscv/codegen/emit.hpp` exposes
  `EdgePublicationMoveIntent::route5_edge_status`,
  `route5_edge_source_agrees`, and `route3_source_memory_agrees`, plus an
  overload of `consume_edge_publication_move_intent(...)` that accepts a
  `const bir::Route5CfgEdgePublicationRecord*`.
- `src/backend/mir/riscv/codegen/emit.cpp` routes that overload through
  `RiscvEdgePublicationMoveAdapter`. The adapter still finds the prepared
  row with `prepare::find_unique_indexed_prepared_edge_publication(...)`; it
  then calls `attach_route5_edge_agreement(...)` to copy the Route 5 status
  and compute `route5_edge_source_agrees`.
- `route5_edge_source_agrees_with_prepared_publication(...)` compares the
  Route 5 edge row against the prepared publication's predecessor label,
  successor label, destination value name/type, source value kind/type, source
  producer kind, and source value. For `LoadLocal`, it requires
  `Route5PublicationStatus::MemorySource`, available source-memory identity,
  and `route3_source_memory_agrees_with_prepared_publication(...)`.
- `route3_source_memory_agrees_with_prepared_publication(...)` requires a
  `LoadLocal` prepared source producer, available prepared source-memory
  access, a Route 3 `LoadLocal` record for the same result value/type/name,
  matching address-space/volatility/offset/size/align fields, and matching
  base identity.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
  function `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback`
  builds a Route 5 edge index with
  `bir::route5_build_edge_join_source_index(...)`, fetches the same-edge row
  with `bir::route5_find_cfg_edge_publication(...)`, and checks a scalar
  available Route 5 row against a prepared publication. The agreeing case
  records `route5_edge_status == Route5PublicationStatus::Available` and
  `route5_edge_source_agrees == true`.
- The same test builds a dynamic `LoadLocal` source-memory row with
  `bir::route5_cfg_edge_publication_record(...)` and confirms
  `Route5PublicationStatus::MemorySource`,
  `source_memory_identity_available`, matching Route 3 `LoadLocal`
  instruction/offset/size fields, `route5_edge_source_agrees == true`, and
  `route3_source_memory_agrees == true`.
- `src/backend/mir/query.cpp`
  `find_bir_cfg_edge_publication_source_identity(...)` remains the shared MIR
  query facade for Route 5/BIR edge-publication source identity. It builds a
  small Route 5 function, calls `bir::route5_build_edge_join_source_index(...)`
  and `bir::route5_find_cfg_edge_publication(...)`, then maps the edge record
  into MIR identity/status output. This supports the same semantic fact family
  but is not the public riscv emission authority.

Rows that prove only diagnostic agreement, not target output authority:

- Scalar Route 5 `Available` agreement proves that the same predecessor,
  successor, destination value, source value, and source producer can be
  compared against the prepared publication.
- Dynamic `LoadLocal` Route 5 `MemorySource` agreement proves that Route 5 can
  carry a Route 3 source-memory identity and that riscv can record agreement
  against the prepared source-memory publication.
- Mismatched Route 5 destination type (`NoMatch`), absent/wrong predecessor
  (`NoSource`), mismatched Route 3 memory offset, and incomplete Route 3 memory
  identity are diagnostic rows only. The riscv helper records disagreement but
  still preserves prepared-backed output/fallback behavior.

Compatibility-owned or target-policy-owned rows:

- RISC-V `EdgePublicationMoveIntentStatus::{MissingSharedLookups,MissingPublication,UnsupportedPublication,UnsupportedSourceHome,UnsupportedDestinationHome,Available}`
  remain prepared/riscv compatibility rows.
- Exact instruction text such as `mv a1, a0`, `lw a1, 12(s2)`, `li`, `addi`,
  `ld`, and `sw`; register choices such as `a0`, `a1`, `s2`, `t0`, and `t6`;
  source/destination-home acceptance; stack-source width/offset rules;
  pointer-base materialization; scratch-register policy; helper/oracle names;
  fallback strings; and output formatting remain target policy or prepared
  compatibility, not Route 5 semantic authority.
- `check_load_local_dynamic_stack_source_exposes_shared_memory_access(...)`
  proves prepared dynamic source-memory publication and riscv `lw` emission
  behavior, but its `lw a1, 12(s2)` output is target policy. It is supporting
  evidence only when paired with the later Route 5/Route 3 agreement rows.

Blockers:

- No riscv-specific blocker for Step 4. RISC-V has explicit diagnostic fields
  for the selected Route 5 CFG-edge publication source identity and tests for
  agreement, mismatch, absent, incomplete, and prepared-fallback rows.
- The Step 3 x86 classification remains blocked: x86 still lacks a direct or
  indirect Route 5/BIR agreement consumer that joins the same-edge prepared
  publication with `Route5CfgEdgePublicationRecord` or
  `BirCfgEdgePublicationSourceIdentity` and rejects disagreement.

## Suggested Next

Execute `plan.md` Step 5, "Build the Fail-Closed Proof Matrix", as an
analysis-only packet for the same selected Route 5 CFG-edge publication source
identity.

## Watchouts

- Keep source idea 251 unchanged unless durable intent truly changes.
- Do not implement an adapter during this blocker map.
- Step 5 should turn the x86 blocker plus riscv diagnostic-only evidence into
  explicit duplicate, mismatch, unsupported, prepared-only, fallback, and
  policy-sensitive fail-closed rows.
- Keep the x86 result classified as blocked unless a later implementation
  introduces a Route 5/BIR agreement consumer or MIR query facade for x86.
- Preserve prepared edge-publication lookup/status, helper/oracle names,
  fallback publication rows, x86 dispatch/status/module output, riscv status
  fields, and riscv/x86 instruction/output strings as compatibility-owned
  unless direct route/BIR agreement proves otherwise.
- Treat testcase-shaped shortcuts, expectation weakening, helper renames, and
  output rewrites as route drift.

## Proof

No build or test proof required; analysis-only packet. AST/focused inspection
used:

- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp consume_edge_publication_move_intent build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp consume_edge_publication_move_intent build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/emit.cpp route5_edge_source_agrees_with_prepared_publication build/compile_commands.json`
- focused `rg` and code inspection over `src/backend/mir/riscv/codegen/emit.*`,
  `src/backend/mir/query.cpp`, `src/backend/bir/bir.*`, and
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Local validation: `git diff --check -- todo.md`.
