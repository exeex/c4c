Status: Active
Source Idea Path: ideas/open/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Implement The Agreement-Gated Adapter

# Current Packet

## Just Finished

Step 2 implemented one local agreement-gated adapter around the selected
AArch64 dispatch value materialization same-block global-load identity read in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`.

Adapter boundary:

- The `LoadGlobalInst` producer branch now obtains the existing prepared
  same-block global-load answer first through
  `prepare::find_prepared_same_block_global_load_access(...)`.
- `route3_agreed_same_block_global_load_access(...)` asks Route 3 for
  `BirSameBlockGlobalLoadAccessIdentity` only after a prepared answer exists.
- Route 3 is accepted only when it agrees with the prepared answer on semantic
  identity: same block, same producer instruction/index and before-index,
  `LoadGlobal` node kind, `GlobalSymbol` base kind, result/root value name and
  type, and global symbol identity.
- Absent, invalid, mismatched, prepared-only, policy-sensitive, or
  non-agreement Route 3 facts keep the existing prepared fallback and final
  `emit_prepared_global_load_to_register(...)` behavior.
- Address formation, address-space/volatile flags, byte offsets, size/align,
  base-plus-offset capability, relocation spelling, register allocation, value
  homes, wrappers, diagnostics, materialization, final operands, and target
  emission policy remain owned by the prepared/target path.

## Suggested Next

Execute Step 3 by reviewing whether the existing nearby same-feature proof is
enough for fallback and public compatibility, then add only focused tests if
the supervisor wants explicit new coverage beyond the green delegated subset.

## Watchouts

- Do not delete, privatize, rename, or hide `memory_accesses`,
  `PreparedFunctionLookups`, or `PreparedBirModule`.
- Do not move address formation, materialization, relocation, final operands,
  value homes, wrappers, diagnostics, fallback, or target emission policy into
  Route 3 ownership.
- The adapter deliberately does not extend the existing `globals.cpp`
  `prepared_current_global_load_access(...)` helper because that helper
  compares policy-bearing fields outside this selected semantic reader.
- The delegated proof exercises the existing instruction-dispatch fallbacks for
  absent/non-global/mismatched/policy-sensitive Route 3 global-load facts and
  the helper/oracle same-block global-load rows, but this packet did not add
  new test cases.

## Proof

Supervisor-selected proof ran and passed:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure > test_after.log
```

Result: 3/3 tests passed in `test_after.log`
(`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_instruction_dispatch`, and
`backend_prepared_lookup_helper`).
