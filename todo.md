Status: Active
Source Idea Path: ideas/open/250_phase_f3_route3_memory_source_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove or Block x86 Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 3, "Prove or Block x86 Evidence", as an
analysis-only x86 classification for the selected Route 3 `LoadLocal`
result/source-memory identity fact.

Result: x86 evidence is blocked for the selected fact.

The selected semantic fact remains Route 3 `LoadLocal` result/source-memory
identity agreement for a prepared dynamic source-memory row: a
`Route3MemoryAccessRecord` for a same-function, same-block `LoadLocal` source
must identify the same memory access as prepared publication
`PreparedEdgePublication::source_memory_access` when both sides describe the
same loaded result value and source-memory fields. Step 3 found no x86 direct
or indirect consumer for that Route 3 identity. Current x86 memory behavior is
prepared-addressing/target-policy backed.

Concrete x86 consumer rows that block semantic classification:

- `src/backend/mir/x86/module/module.cpp:3817`
  `render_prepared_frame_slot_memory_operand(...)` calls
  `prepare::find_prepared_memory_access(...)`, requires prepared
  `FrameSlot`, frame-slot id, nonnegative byte offset,
  `can_use_base_plus_offset`, prepared size, and then renders target stack
  operand spelling through `render_stack_memory_operand(...)`.
- `src/backend/mir/x86/module/module.cpp:3990`
  `render_prepared_local_slot_statement_memory_operand(...)` calls
  `prepare::find_prepared_memory_access(...)`, delegates to the frame-slot
  renderer, and throws `"local-slot statement drifted from prepared frame-slot
  access"` when prepared result/stored names disagree with the expected BIR
  statement.
- `src/backend/mir/x86/module/module.cpp:4057`
  `render_prepared_local_slot_i32_guard_prefix(...)` consumes the prepared
  local-slot statement memory renderer for stores and `LoadLocal` loads before
  guard emission; it records prepared memory operands and later reloads them
  through prepared value-home/register policy.
- `src/backend/mir/x86/module/module.cpp:5472` and `:5489`
  `find_supported_same_module_global_span(...)` and
  `render_prepared_same_module_global_memory_operand(...)` require prepared
  `GlobalSymbol`, expected result/stored names, nonnegative offset,
  base-plus-offset policy, size, and same-module global span before emitting
  target global memory operands.
- `src/backend/mir/x86/module/module.cpp:5551`
  `append_prepared_i32_guard_compare_source(...)` consumes prepared same-module
  global memory rendering for `StoreGlobal` and `LoadGlobal`; this is target
  global-span and prepared-addressing evidence, not Route 3 `LoadLocal`
  source-memory identity evidence.

AST-backed confirmation:

- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/x86/module/module.cpp
  render_prepared_frame_slot_memory_operand build-x86/compile_commands.json`
  reported callees `find_prepared_memory_access`,
  `find_prepared_frame_slot_by_id`,
  `throw_prepared_value_location_handoff_error`, and
  `render_stack_memory_operand`; no Route 3 or MIR query facade callee.
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/x86/module/module.cpp
  render_prepared_local_slot_statement_memory_operand
  build-x86/compile_commands.json` reported callees
  `find_prepared_memory_access`,
  `render_prepared_frame_slot_memory_operand`, and
  `throw_prepared_value_location_handoff_error`; no Route 3 or MIR query
  facade callee.
- `c4c-clang-tool-ccdb function-callees
  /workspaces/c4c/src/backend/mir/x86/module/module.cpp
  render_prepared_same_module_global_memory_operand
  build-x86/compile_commands.json` reported callees
  `find_prepared_memory_access`, `prepared_link_name`,
  `find_supported_same_module_global_span`,
  `render_global_i32_memory_operand`, and
  `render_global_ptr_memory_operand`; no Route 3 or MIR query facade callee.
- `c4c-clang-tool-ccdb function-callers
  /workspaces/c4c/src/backend/mir/x86/module/module.cpp
  render_prepared_local_slot_statement_memory_operand
  build-x86/compile_commands.json` reported prepared local-slot consumers:
  `append_prepared_symbol_call_local_i32_function`,
  `find_prepared_local_slot_compare_load`,
  `render_prepared_local_slot_i32_guard_prefix`,
  `append_prepared_local_slot_return_function`,
  `append_prepared_local_slot_scalar_guard_block`, and
  `append_prepared_local_slot_immediate_guard_function`.
- `c4c-clang-tool-ccdb function-callers
  /workspaces/c4c/src/backend/mir/x86/module/module.cpp
  render_prepared_same_module_global_memory_operand
  build-x86/compile_commands.json` reported
  `append_prepared_i32_guard_compare_source` as the caller.

Targeted text-search confirmation:

- `rg -n
  "route3_find_memory_access_record|find_bir_memory_access_identity|find_bir_same_block_load_local_source_identity|find_bir_same_block_load_local_stored_value_source_identity|Route3MemoryAccessRecord|source_memory_access|memory_source|PreparedEdgePublicationSourceMemoryAccessStatus|find_prepared_memory_access|PreparedFunctionLookups|memory_accesses"
  src/backend/mir/x86 tests/backend/bir/backend_x86*
  tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  returned x86 hits for prepared lookup APIs and tests, but no
  `src/backend/mir/x86` Route 3 memory identity or source-memory publication
  consumer.
- `tests/backend/bir/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3443`
  through `:3462` mutates prepared memory access addresses and expects x86
  prepared emission to follow the authoritative prepared frame-slot accesses.
  The test names the route as
  `"bounded multi-defined call local-slot frame-access drift route"`.
- `tests/backend/bir/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3480`
  through `:3500` mutates prepared load-result homes and expects x86 prepared
  emission to follow authoritative prepared load homes.
- `tests/backend/bir/backend_x86_handoff_boundary_local_i16_guard_test.cpp:497`
  through `:507` checks the prepared i16/i64 subtract fixture by direct
  `prepare::find_prepared_memory_access(...)` rows, while `:525` through
  `:532` checks the raw BIR carriers as `StoreLocalInst` / `LoadLocalInst`.
  This preserves prepared-memory handoff and raw carrier shape, but does not
  prove x86 consumes Route 3 source-memory identity.

x86 output/status rows that must remain compatibility-owned:

- Stack memory operand text and size spelling: `BYTE`, `WORD`, `DWORD`, `QWORD`
  rows such as `WORD PTR [rsp]`, `DWORD PTR [rsp + 4]`, and
  `QWORD PTR [rsp + 8]`.
- Prepared frame-slot ids, frame offsets, byte offsets, size bytes,
  base-plus-offset legality, and target render helpers.
- Prepared result/stored-value name drift checks and handoff error text such as
  `"local-slot statement drifted from prepared frame-slot access"` and
  `"has no prepared frame-slot access"`.
- Prepared value-home/register choices for loaded values and call/local-slot
  lanes.
- Same-module global symbol lookup, global span policy, prepared link-name
  spelling, and global operand text.
- Existing helper/oracle names and prepared memory lookup/status behavior,
  including missing, incomplete, duplicate, invalid-name, prepared-only, and
  fallback rows.

Blocker:

- The exact blocker is the absence of any x86 consumer that reads the selected
  Route 3 `LoadLocal` `Route3MemoryAccessRecord` or MIR memory query facade
  and compares it with prepared `source_memory_access`. The closest x86
  consumer row is prepared local-slot/global memory rendering in
  `src/backend/mir/x86/module/module.cpp`, but that row consumes
  `PreparedAddressingFunction` / `PreparedMemoryAccess` and target policy
  directly. It cannot be classified as proof of Route 3 result/source-memory
  identity.

## Suggested Next

Execute Step 4 by rechecking RISC-V evidence against the same selected Route 3
`LoadLocal` result/source-memory identity fact. The next packet should separate
RISC-V identity agreement flags from prepared-backed `lw` output and target
policy rows.

## Watchouts

- x86 is blocked, not non-applicable: it has prepared memory consumers, but no
  selected Route 3 source-memory identity consumer.
- Do not count x86 operand text, prepared frame-slot/global address following,
  or prepared drift tests as semantic Route 3 proof.
- A future adapter would need a new x86-local consumer or explicit agreement
  bridge before prepared memory lookup/status rows could become compatibility
  mirrors for this fact.
- RISC-V Step 4 should reuse the same selected fact from Step 2 and should not
  broaden to generic memory parity or final instruction text.

## Proof

No build/test proof required by the delegated packet. Analysis-only validation
used the targeted `rg` and `c4c-clang-tool-ccdb` commands recorded above. No
`test_after.log` produced because proof was explicitly not required.
