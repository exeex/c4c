# Current Packet

Status: Active
Source Idea Path: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add The Shared MIR Printer Boundary

## Just Finished

Completed `plan.md` Step 3 by adding the shared MIR printer boundary under
`src/backend/mir/printer.*`. The common printer now walks
`MachineModule -> MachineFunction -> MachineBlock -> MachineInstruction`,
passes structural print context to a target-owned spelling interface, accepts
only unindented instruction line payloads from that interface, owns instruction
indentation and newline joining, and fails closed with a structural diagnostic
instead of returning partial assembly when target spelling is unsupported or
preformatted.

Focused coverage now instantiates the boundary with both a fake target printer
and an AArch64-facing temporary bridge that strips the existing target
`print_machine_instruction_node(...)` output back to line payloads before the
common printer applies indentation/newlines; public AArch64 assembly routing
remains unchanged for the later Step 5 packet.

## Suggested Next

Execute Step 4 as the next narrow packet: add or adjust the target adapter
surface needed before Step 5 can route public AArch64 assembly through the
common MIR printer.

## Watchouts

- Keep idea 229 as a follow-up markdown-shard conversion route unless the
  supervisor changes lifecycle direction.
- Do not grow `src/backend/mir/aarch64/codegen/machine_printer.*` as the
  permanent terminal assembly owner; it is still only the AArch64 instruction
  spelling delegate.
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
- `flatten_instructions(...)` still exists for current flat-vector callers; the
  new shared printer uses direct function/block/instruction iteration and Step 5
  should avoid flattening when routing public assembly.
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
