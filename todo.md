# Current Packet

Status: Active
Source Idea Path: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current MIR Carrier And Assembly Route

## Just Finished

Completed `plan.md` Step 1 audit of the post-228 AArch64 MIR-to-assembly path.
Current public assembly route:
`emit_aarch64_lir_module_entry(...)` / `emit_aarch64_bir_module_entry(...)` in
`src/backend/backend.cpp` prepare semantic BIR, call
`print_aarch64_prepared_machine_nodes(...)`, build the AArch64 prepared module
through `aarch64::api::build_prepared_module(...)`, then walk
`built.module->functions`. For each non-empty function, `backend.cpp` emits the
`.text`, `.globl`, `.type`, function label, `.size`, and `.note.GNU-stack`
assembly scaffolding, flattens `function.mir` with
`mir::flatten_instructions(...)`, copies each `MachineInstruction::target` into
a `std::vector<aarch64::codegen::InstructionRecord>`, and calls
`aarch64::codegen::print_machine_instruction_nodes(...)`.

Prepared/MIR construction route:
`src/backend/mir/aarch64/codegen/emit.cpp` lowers prepared functions into
`module::MachineModule` via `lower_prepared_functions(...)`, then derives
`module::FunctionRecord` compatibility mirrors with
`derive_compatibility_function_records(...)` and
`derive_compatibility_projection(...)`. The canonical carrier is
`module::Module::mir`; `FunctionRecord::machine_nodes` is derived compatibility
state.

Flat compatibility callers/users found:
`src/backend/backend.cpp` is the active public flat-vector assembly caller via
`mir::flatten_instructions(...)` plus `print_machine_instruction_nodes(...)`.
`src/backend/mir/aarch64/codegen/compatibility_projection.cpp` derives
`FunctionRecord::machine_nodes` from selected non-return target records.
`src/backend/mir/aarch64/codegen/machine_printer.*` exposes the flat
`std::vector<InstructionRecord>` printer API. Tests directly inspect or depend
on flat compatibility state in
`backend_aarch64_return_lowering_test.cpp`,
`backend_aarch64_function_traversal_test.cpp`,
`backend_aarch64_instruction_dispatch_test.cpp`,
`backend_aarch64_branch_control_lowering_test.cpp`, and
`backend_aarch64_prepared_scalar_alu_records_test.cpp`; printer tests call the
flat vector API in `backend_aarch64_machine_printer_test.cpp`.

Target-local formatting responsibilities currently in
`machine_printer.*`: selected-node validation and failure diagnostics; target
mnemonic lookup for primary, auxiliary, and branch mnemonics; physical register
spelling through `abi::register_name(...)`; immediate spelling as `#imm`;
memory operand spelling for frame-slot and pointer-value base-plus-offset
forms; block label spelling as `.LBB<function>_<block>`; target instruction
line spelling for spill/reload, branch, store, scalar add/sub, and return
records; flat instruction order, per-instruction indentation, and instruction
newline emission inside `print_machine_instruction_nodes(...)`.

## Suggested Next

Execute Step 2: normalize `src/backend/mir/mir.hpp` as the common hierarchical
stream contract while keeping target instruction payloads target-owned and
leaving flat helpers/projections explicitly compatibility-only.

Recommended narrow proof command for the first code-changing packet:

```bash
cmake --build build --target c4c_backend backend_aarch64_mir_carrier_test backend_aarch64_machine_printer_test c4cll && ctest --test-dir build -R '^(backend_aarch64_mir_carrier|backend_aarch64_machine_printer|backend_cli_aarch64_asm_external_return_add_smoke)$' --output-on-failure
```

The C fixture `backend_cli_aarch64_asm_external_return_add_smoke` is the
smallest existing public assembly proof that exercises selected AArch64 scalar
machine nodes through `--codegen asm`; it is configured only when the external
AArch64 assembler/toolchain is available.

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

## Proof

Audit-only metadata update. No build or CTest run, and no root proof logs
created or modified.
