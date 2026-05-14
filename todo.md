# Current Packet

Status: Active
Source Idea Path: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Adapt AArch64 Spelling To The Target Printer Interface

## Just Finished

Completed `plan.md` Step 4 by moving AArch64 instruction spelling behind a
production target-printer adapter surface. `machine_printer.*` now exposes
`MachineInstructionPrinter`, which implements
`mir::TargetInstructionPrinter<InstructionRecord>`, plus
`print_machine_instruction_line_payloads(...)` for unindented target-owned
instruction payloads. The existing `print_machine_instruction_node(s)` APIs
remain compatibility wrappers that apply the legacy indentation/newline shape,
so public AArch64 assembly routing is still unchanged for Step 5.

Existing selected-node validation, target-owned mnemonic/register/immediate/
memory/branch-label spelling, and unsupported-family diagnostics were preserved.
The focused machine-printer coverage now uses the production adapter directly
instead of a test-local bridge that stripped legacy assembly back into payloads.

## Suggested Next

Execute Step 5 as the next narrow packet: route public AArch64 assembly through
the shared MIR printer using the new production AArch64 target spelling
adapter, while preserving existing section/function scaffolding behavior.

## Watchouts

- Keep idea 229 as a follow-up markdown-shard conversion route unless the
  supervisor changes lifecycle direction.
- Do not grow `src/backend/mir/aarch64/codegen/machine_printer.*` as the
  permanent terminal assembly owner; it is still only the AArch64 instruction
  spelling delegate.
- Treat `FunctionRecord::machine_nodes` and other flat views as compatibility
  projections, not as the desired public assembly carrier.
- `backend.cpp` currently owns public function/section scaffolding while
  `machine_printer.*` owns target instruction line spelling; Step 5 should
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
