Status: Active
Source Idea Path: ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select The Route 3 Memory/Source Reader

# Current Packet

## Just Finished

Step 1 selected one Route 3 `memory_accesses` reader for the agreement-gated
adapter: the same-block global-load identity read in
`emit_value_publication_to_register(...)` at
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`, where
the `LoadGlobalInst` producer path calls
`mir::find_bir_same_block_global_load_access_identity(...)` before falling
back to the prepared same-block global-load lookup.

Selected reader details:

- Selected reader: `mir::find_bir_same_block_global_load_access_identity(...)`
  in the AArch64 dispatch value materialization `LoadGlobalInst` producer
  branch.
- Route fact to consume: `BirSameBlockGlobalLoadAccessIdentity` only for
  same-block source identity: root/result value identity and type, producing
  instruction pointer/index and before-index compatibility, `LoadGlobal`
  memory-access node kind, `GlobalSymbol` base kind, block label, and global
  symbol identity.
- Current prepared lookup answer: the existing prepared fallback builds a
  `PreparedSameBlockScalarProducer` with
  `prepared_shape_same_block_scalar_producer(...)`, then calls
  `prepare::find_prepared_same_block_global_load_access(...)` to obtain
  `PreparedSameBlockGlobalLoadAccess{load_global, access}` from
  `PreparedFunctionLookups::memory_accesses`.
- Fallback path: if the Route 3 fact is absent, invalid, ambiguous, mismatched
  against the prepared same-block global-load answer, or asks for policy data,
  keep the existing prepared path and final
  `emit_prepared_global_load_to_register(...)` behavior unchanged.
- Semantic-only boundary: Step 2 must use Route 3 only to confirm which
  same-block `LoadGlobalInst` produced the value. It must not consume Route 3
  address-space, volatile, byte-offset, size/align, base-plus-offset,
  relocation, register, value-home, wrapper, diagnostic, materialization, or
  final operand policy as owned facts.
- Nearby same-feature cases: integer global-load materialization in
  `backend_aarch64_instruction_dispatch`, FP same-block global-load identity in
  `backend_aarch64_prepared_memory_operand_records`, prepared helper/oracle
  same-block global-load rows in `backend_prepared_lookup_helper`, and negative
  same-feature cases for before-producer, root type mismatch, non-global root,
  string-constant/non-global base, mismatched global symbol, and policy-flag
  fallback.

## Suggested Next

Execute Step 2 by adding one local agreement-gated adapter around the selected
dispatch value materialization same-block global-load identity read. The
adapter should first obtain the current prepared same-block global-load answer,
accept the Route 3 identity only when it agrees on the semantic identity fields
above, and otherwise fall back to the existing prepared path.

## Watchouts

- Keep the slice to one reader and one agreement-gated adapter boundary.
- Do not delete, privatize, rename, or hide `memory_accesses`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not move address formation, materialization, relocation, final operands,
  value homes, wrappers, diagnostics, fallback, or target emission policy into
  Route 3 ownership.
- Preserve prepared lookup delivery, helper-oracle names/status labels,
  prepared printer/debug output, route-debug output, wrapper output, expected
  strings, supported-path contracts, and baselines.
- Treat expectation rewrites, helper renames, unsupported downgrades, baseline
  refreshes, facade-only moves, aggregate reshuffles, and named-case shortcuts
  as reject signals.
- Do not select or extend the existing `globals.cpp`
  `prepared_current_global_load_access(...)` helper for this packet: its
  agreement helper currently compares policy-bearing fields such as
  address-space, volatility, byte offsets, size/align, and base-plus-offset
  capability, so it is not the selected semantic-only reader.
- Do not let the Step 2 adapter own
  `emit_prepared_global_load_to_register(...)`,
  `prepared_load_global_address_policy(...)`, address materialization,
  relocation spelling, register allocation, value homes, or final operands.

## Proof

Docs/lifecycle selection only. No build or CTest was run because no
implementation files changed, and `test_after.log` was not required.

Source inspection commands used:

- `git status --short`
- `rg -n "memory_accesses|PreparedMemoryAccessLookups|find_prepared_memory_access|find_prepared_global_load_access|find_prepared_same_block_global_load_access|load-local|load local|local source" src/backend/mir/aarch64 src/backend -g'*.cpp' -g'*.hpp'`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prepared_lookups.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/globals.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/fp_value_materialization.cpp build/compile_commands.json`
- targeted source reads of `globals.cpp`, `dispatch_value_materialization.cpp`,
  `fp_value_materialization.cpp`, `prepared_lookups.cpp`, and the relevant
  backend test files.

Recommended Step 2/3 narrow proof subset after implementation:

```bash
cmake --build --preset default
ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure
```
