Status: Active
Source Idea Path: ideas/open/329_native_object_emission_umbrella.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current Backend Emission Architecture

# Current Packet

## Just Finished

Completed Step 1 research: inspected current RV64 and AArch64 backend emission
architecture and recorded object-emission integration seams without touching
implementation files.

Inspected commands/files:

- `git status --short`
- `rg --files src/backend/mir/riscv src/backend/mir/aarch64`
- `rg -n "assemble_target_lir_module|assemble_module\\(|print_prepared_machine_nodes|compile_prepared_module|emit_prepared_module\\(|emit_prepared_lir_module" src -g '*.cpp' -g '*.hpp'`
- `c4c-clang-tool-ccdb list-symbols` on:
  - `src/backend/mir/riscv/codegen/emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
  - `src/backend/mir/aarch64/codegen/module_compile.cpp`
  - `src/backend/mir/aarch64/codegen/asm_emitter.cpp`
  - `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- Targeted reads:
  - `src/backend/backend.hpp`
  - `src/backend/backend.cpp`
  - `src/backend/mir/mir.hpp`
  - `src/backend/mir/printer.hpp`
  - `src/backend/mir/riscv/codegen/emit.hpp`
  - `src/backend/mir/riscv/codegen/emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_module_emit.cpp`
  - `src/backend/mir/riscv/assembler/mod.hpp`
  - `src/backend/mir/riscv/assembler/mod.cpp`
  - `src/backend/mir/riscv/assembler/parser.hpp`
  - `src/backend/mir/riscv/assembler/elf_writer.cpp`
  - `src/backend/mir/riscv/assembler/encoder/mod.cpp`
  - `src/backend/mir/riscv/assembler/README.md`
  - `src/backend/mir/aarch64/module/module.hpp`
  - `src/backend/mir/aarch64/codegen/codegen.hpp`
  - `src/backend/mir/aarch64/codegen/module_compile.hpp`
  - `src/backend/mir/aarch64/codegen/module_compile.cpp`
  - `src/backend/mir/aarch64/codegen/asm_emitter.hpp`
  - `src/backend/mir/aarch64/codegen/asm_emitter.cpp`
  - `src/backend/mir/aarch64/codegen/machine_printer.hpp`
  - `src/backend/mir/aarch64/codegen/machine_printer.cpp`
  - `src/backend/mir/aarch64/codegen/instruction.hpp`
  - `src/backend/mir/aarch64/assembler/mod.hpp`
  - `src/backend/mir/aarch64/assembler/types.hpp`

Current RV64 emission map:

- Public backend entry points in `src/backend/backend.cpp` route RV64 BIR/LIR
  through semantic `lir_to_bir`, `prepare_semantic_bir_pipeline(...)`, then
  `c4c::backend::riscv::codegen::emit_prepared_module(...)`.
- `src/backend/mir/riscv/codegen/emit.cpp` exposes:
  - `emit_prepared_module(...)`, a thin wrapper over
    `emit_prepared_module_text(...)`.
  - `emit_prepared_lir_module(...)`, a tiny direct-LIR special case for the
    hard-coded return-add slice.
  - `assemble_module(...)`, which delegates to generic
    `assemble_target_lir_module(...)`; that still stages text and reports
    `object_emitted = false` for non-x86 targets.
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp` is the real RV64 asm
  writer for prepared BIR. It builds one `std::string`, emits globals through
  `append_prepared_global_storage_asm(...)`, emits `.text`, `.globl`, labels,
  then relies on `append_simple_prepared_bir_function_asm(...)` and
  `append_edge_publication_move_instruction(...)` helpers.
- RV64 does not currently publish a target MIR `MachineModule` stream for the
  prepared route. The compiler-side writer is prepared-fact-to-text, with
  narrower helper structs such as `EdgePublicationMoveIntent` carrying
  route-local facts and final `instruction_text`.
- `src/backend/mir/riscv/assembler/` is a separate text-assembler/object port.
  Its public API accepts `AssembleRequest{asm_text, output_path}` and parses
  `AsmStatement` records with directives, labels, operands, sections, symbols,
  data values, and relocation-shaped operands. The current ELF writer is
  bounded to minimal activation and jal-helper relocation slices, not a general
  compiler object backend.

Current AArch64 emission map:

- Public backend entry points route AArch64 BIR/LIR through semantic
  `lir_to_bir`, `prepare_semantic_bir_pipeline(...)`, clear prepared regalloc
  functions for this route, then call
  `c4c::backend::aarch64::codegen::print_prepared_machine_nodes(...)`.
- `src/backend/mir/aarch64/codegen/module_compile.cpp` exposes
  `build_module(...)` / `compile_prepared_module(...)`, which validate the
  prepared-module handoff, create `module::Module`, lower prepared functions
  into `module::MachineModule`, and derive compatibility flat records.
- `src/backend/mir/aarch64/module/module.hpp` aliases the shared MIR hierarchy:
  `MachineModule`, `MachineFunction`, `MachineBlock`, and `MachineInstruction`
  over target `codegen::InstructionRecord`.
- `src/backend/mir/aarch64/codegen/asm_emitter.cpp` is the terminal asm writer.
  It calls `lower_prepared_functions(...)`, then
  `mir::print_machine_function(function, MachineInstructionPrinter{})`, wraps
  labels/`.globl`/`.type`/`.size`, and appends globals/string constants as text
  directives.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` prints structured
  `InstructionRecord` variants to GNU-style line payloads. The shared
  `mir::print_machine_function(...)` owns block-label printing and indentation.
- `src/backend/mir/aarch64/assembler/` is explicitly documented as an external
  assembler text compatibility surface and future object-emission staging, not
  the internal compile-through bridge from machine instruction nodes.

Shared and non-shared abstractions:

- Shared today: the generic MIR hierarchy in `src/backend/mir/mir.hpp` and
  printer contract in `src/backend/mir/printer.hpp`. AArch64 uses this for its
  terminal asm stream.
- Not shared today: RV64 prepared emission does not use the shared MIR
  hierarchy; it emits text directly from prepared facts. A future shared object
  model cannot assume both targets already expose the same target-instruction
  stream.
- AArch64 `InstructionRecord` already has object-facing vocabulary:
  `RecordSurfaceKind::EncoderInput`, `InstructionFamily::Object`,
  `MachineSideEffectKind::ObjectEmission`, `SymbolOperand`, branch target
  operands, address materialization records, TLS relocation facts, and selected
  machine-node validation.
- RV64 object-facing vocabulary currently lives mostly in the assembler port:
  parsed `AsmStatement`, `Operand`, `Directive`, `SectionInfo`, `DataValue`,
  `PendingReloc`, minimal ELF writer constants, and reference-documented
  `EncodeResult`/`Relocation` concepts. The compiler route needs a structured
  machine/object handoff before it can avoid text round-tripping.

Candidate seams for child ideas:

- Shared object model/API child: define a target-neutral object emission
  handoff for sections, symbols, labels, encoded byte ranges, relocations,
  diagnostics, and final write result. This should sit below target instruction
  selection and above ELF layout, and must not require printing `.s` then
  reparsing it.
- AArch64 child: consume `module::MachineModule<InstructionRecord>` directly
  after `build_module(...)` or `lower_prepared_functions(...)`, using
  `InstructionRecord` payloads plus `mir::MachinePrintContext`-equivalent
  function/block provenance to produce encoded instructions, labels, symbols,
  and typed AArch64 relocations. Preserve the existing
  `print_prepared_machine_nodes(...)` asm route as terminal text.
- RV64 child: introduce a structured RV64 prepared-machine stream or adapter
  beside `emit_prepared_module_text(...)`, rather than driving object emission
  from the existing text string. Candidate near-term owned seams are the helper
  emitters currently producing instruction text and the prepared facts consumed
  by `append_simple_prepared_bir_function_asm(...)`,
  `append_prepared_global_storage_asm(...)`, and
  `append_edge_publication_move_instruction(...)`.
- RV64 assembler/object-port child: reuse or rebuild the parser/ELF writer only
  as appropriate for external `c4c-as` text input. The compiler direct `.o`
  route should share lower-level encode/relocation/ELF facilities, not the
  text parser. Current `elf_writer.cpp` is too slice-shaped to be the compiler
  backend as-is.
- Route/API child: add explicit object staging next to existing
  `emit_*_lir_module_entry(...)` / `emit_*_bir_module_entry(...)` APIs. Today
  `assemble_target_lir_module(...)` returns staged text plus
  `object_emitted = false` for non-x86 targets, so `--codegen obj` needs a
  distinct result contract and target dispatch.

Route differences child ideas must preserve:

- Existing `.s`/asm output must keep using the terminal asm writers and tests.
  Object emission should be additive and must not weaken text-route coverage.
- AArch64 currently clears prepared regalloc functions on its public route and
  depends on AArch64 handoff validation before lowering. Child work must keep
  this target-specific route behavior unless a separate plan changes it.
- RV64 allows only narrowly supported external runtime calls in
  `emit_prepared_module_text(...)`; object work must preserve those rejection
  diagnostics until semantic support expands.
- AArch64 globals/string constants are appended by `asm_emitter.cpp` as textual
  directives after function text. Object emission needs an equivalent
  data-section path from prepared/BIR globals and string constants, not just
  instruction encoding.
- Function/block label identity is structured in shared MIR for AArch64 but
  string-spelled in RV64 text emission. A shared object model should use
  stable IDs where available and require explicit RV64 label/symbol mapping.

## Suggested Next

Execute Step 2 by inspecting backend runtime and c-testsuite route wiring:
where `.s` output is compiled/linked today, how backend tests distinguish
target/route/output form, and where object-route tests can sit beside asm-route
tests.

## Watchouts

Do not turn the existing RV64 text assembler into the compiler's direct `.o`
path by round-tripping terminal asm. AArch64 has a structured MIR seam now;
RV64 likely needs a child idea for a structured prepared-machine/object handoff
before serious object emission can be shared.

## Proof

Research-only packet; no build or test validation required and no
`test_after.log` was written. Proof is the inspected file/command list recorded
above in `todo.md`.
