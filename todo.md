# Current Packet

Status: Active
Source Idea Path: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Normalize The Common MIR Stream Contract

## Just Finished

Completed `plan.md` Step 2 by normalizing `src/backend/mir/mir.hpp` around the
common hierarchical MIR stream contract. The shared carrier now documents
`MachineModule -> MachineFunction -> MachineBlock -> MachineInstruction` as the
authoritative common shape, keeps each target-owned instruction payload opaque
in `MachineInstruction::target`, and exposes `walk_instructions(...)` helpers
for common hierarchical walking without flattening away function/block context.

Flat compatibility state remains available but is explicitly marked as
compatibility-only: `MachineNode`, legacy generic `Function`/`Block`, and
`flatten_instructions(...)` now point callers toward hierarchical walking, with
`flatten_compatibility_instructions(...)` as the named flat-vector helper for
existing printer/projection routes. `backend_aarch64_mir_carrier_test.cpp`
covers hierarchy walking, AArch64 target payload ownership, successor metadata,
origin preservation, and compatibility flattening.

## Suggested Next

Execute Step 3 as a narrow shared traversal/printer-boundary packet: route the
public assembly path toward the common hierarchical MIR stream while preserving
target-local instruction spelling in the AArch64 printer.

## Watchouts

- Keep idea 229 as a follow-up markdown-shard conversion route unless the
  supervisor changes lifecycle direction.
- Do not grow `src/backend/mir/aarch64/codegen/machine_printer.*` as the
  permanent terminal assembly owner.
- Treat `FunctionRecord::machine_nodes` and other flat views as compatibility
  projections, not as the desired public assembly carrier.
- `backend.cpp` currently owns public function/section scaffolding while
  `machine_printer.*` owns target instruction line spelling; Step 3/5 should
  preserve that distinction when moving traversal/newline/indent policy under
  shared MIR.
- `compatibility_projection.cpp` currently excludes return records from
  `FunctionRecord::machine_nodes`, but the public assembly route does not use
  that field; it flattens `function.mir` directly and therefore includes return
  records via `MachineInstruction::target`.
- `flatten_instructions(...)` still exists for current flat-vector callers; new
  shared MIR traversal should prefer `walk_instructions(...)` or direct
  function/block iteration.
- Separate idea file `ideas/open/230_aarch64_c_testsuite_backend_full_scan.md`
  exists outside this packet and was not touched.

## Proof

Delegated proof passed and wrote CTest output to `test_after.log`:

```bash
cmake --build build --target c4c_backend backend_aarch64_mir_carrier_test backend_aarch64_machine_printer_test c4cll && ctest --test-dir build -R '^(backend_aarch64_mir_carrier|backend_aarch64_machine_printer|backend_cli_aarch64_asm_external_return_add_smoke)$' --output-on-failure > test_after.log
```

CTest subset: `backend_aarch64_machine_printer`,
`backend_aarch64_mir_carrier`, and
`backend_cli_aarch64_asm_external_return_add_smoke`, all passed.
