# Current Packet

Status: Active
Source Idea Path: ideas/open/254_aarch64_globals_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Survey Current Globals Ownership

## Just Finished

Step 1 - Survey Current Globals Ownership completed as a survey-only packet.

Current globals-family selected-node construction ownership:
- `instruction.hpp` owns the public record surface: `AddressMaterializationKind`,
  `PreparedAddressMaterializationRecordError`, `AddressMaterializationRecord`,
  `PreparedAddressMaterializationRecordResult`, and
  `PreparedAddressMaterializationInstructionRecordResult`.
- `instruction.hpp` also owns the public construction entry points:
  `make_address_materialization_instruction`,
  `make_prepared_address_materialization_record`, and
  `make_prepared_address_materialization_instruction_record`.
- `instruction.cpp` owns globals-specific kind selection and prepared-fact
  validation through `selected_address_materialization_kind`,
  `validate_address_materialization_identity`,
  `make_address_record_from_prepared_materialization`,
  `address_materialization_selection_status`, and the public construction
  wrappers.
- `instruction.cpp` also owns generic machine-instruction packaging in
  `make_address_materialization_instruction`; that packaging touches operands,
  defs/uses, opcode selection, selection status, provenance, and payload.

Current globals-family lowering ownership:
- `dispatch.cpp` owns the route from prepared address materialization facts to
  selected machine nodes through `lower_address_materialization`.
- `dispatch.cpp` owns lowering diagnostics for this family through
  `append_address_materialization_diagnostic` and
  `address_materialization_error_message`.
- `dispatch_prepared_block` remains the broad router and scalar-register state
  boundary: after `lower_address_materialization`, it records the selected
  result register with `record_emitted_scalar_register` before pushing the
  machine instruction.

Current globals-family printer ownership:
- `machine_printer.cpp` owns relocation spelling helpers used by globals:
  `relocation_operand`, `prefixed_relocation_operand`, and
  `tls_relocation_prefix`.
- `machine_printer.cpp` owns direct, GOT, string, label, and TLS address
  spelling in `print_address_materialization`.
- `print_machine_instruction_line_payloads` is the generic printer dispatcher
  that routes `AddressMaterializationRecord` payloads to the globals-specific
  printer body.

Family-neutral boundaries that should remain outside `globals.cpp` /
`globals.hpp`:
- Generic record enums, instruction variants, opcode naming, payload variant
  plumbing, operand constructors, and generic `InstructionRecord` metadata
  belong in `instruction.*` unless an interface split proves necessary.
- `dispatch_prepared_block` iteration, visited-operation accounting, call/memory
  routing order, and scalar-register tracking should remain in `dispatch.cpp`.
- Generic printer validation, `MachineInstructionPrinter` dispatch, register
  spelling, `bad_header`, and shared unsupported/printed result helpers should
  remain in `machine_printer.cpp`.
- Prepared facts, allocation/storage authority, and name-table lookup remain in
  the existing `prepare` and BIR owners; globals code should consume those facts
  rather than invent lookup or allocation policy.

First code-moving packet identified:
- After creating the owner shell, move globals-family record construction from
  `instruction.cpp` into `globals.cpp` and expose the minimal declarations from
  `globals.hpp`: `selected_address_materialization_kind`,
  `validate_address_materialization_identity`,
  `make_address_record_from_prepared_materialization`, and the public prepared
  address-materialization construction wrappers as needed.
- The owner-shell packet must add
  `src/backend/mir/aarch64/codegen/globals.cpp` to the explicit AArch64 codegen
  source list in `src/backend/CMakeLists.txt`; `src/codegen/CMakeLists.txt`
  globbing does not cover this backend library.
- Keep `AddressMaterializationRecord`, the public enums/result structs, generic
  operand/effect creation, and `make_address_materialization_instruction` in
  `instruction.*` for that first movement unless the compile boundary requires a
  smaller forwarding shim.

## Suggested Next

Delegate Step 2 to create `src/backend/mir/aarch64/codegen/globals.hpp` and
`src/backend/mir/aarch64/codegen/globals.cpp` as an empty/minimal compiled owner
shell, and add `src/backend/mir/aarch64/codegen/globals.cpp` to the explicit
`C4C_BACKEND_SOURCES` list in `src/backend/CMakeLists.txt` next to the other
AArch64 codegen sources.

## Watchouts

- Keep this plan scoped to globals shard redistribution.
- Do not downgrade tests, mark supported globals paths unsupported, or rewrite
  expectations to claim progress.
- Preserve direct, GOT, label, string-constant, and TLS address materialization
  behavior through structured prepared facts and machine instruction records.
- The first real movement should not pull in generic instruction packaging,
  dispatch loop control, scalar-register tracking, or generic printer dispatch.
- Existing focused coverage for this family is concentrated in
  `backend_aarch64_prepared_memory_operand_records`,
  `backend_aarch64_machine_printer`, and `backend_aarch64_instruction_dispatch`.

## Proof

No build or test run was required for this survey-only packet, so
`test_after.log` was not refreshed.

Focused proof command for the first implementation/code-moving packet:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_(prepared_memory_operand_records|machine_printer|instruction_dispatch)$'
```
