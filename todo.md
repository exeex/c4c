Status: Active
Source Idea Path: ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Wire The Public c4cll Assembly-Output Route

# Current Packet

## Just Finished

Step 3 wired the public `c4cll --codegen asm --target aarch64-linux-gnu`
route through the real source-to-backend pipeline.

Completed work:
- Added explicit public backend entrypoints
  `emit_aarch64_bir_module_entry(...)` and
  `emit_aarch64_lir_module_entry(...)`.
- The AArch64 route now lowers `.c` input through HIR -> LIR -> semantic BIR
  -> prepared BIR -> AArch64 module build -> selected
  `FunctionRecord::machine_nodes` before calling
  `codegen::print_machine_instruction_nodes(...)`.
- The route fails closed with an explicit diagnostic when the current source
  pipeline reaches the AArch64 machine-node printer but produces no selected
  printable machine nodes.
- Added public CLI coverage for
  `--codegen asm --target aarch64-linux-gnu input.c -o out.s` proving that the
  no-machine-node case fails clearly and does not write fallback LLVM IR or a
  fixture `.s` file.

## Suggested Next

Implement Step 4 by adding external-toolchain smoke coverage for the public
AArch64 `.s` route once a source case or bounded backend fixture can produce
selected printable machine nodes through the real module pipeline.

## Watchouts

- The `.s` printer is output-only; do not parse printed assembly back into
  backend semantics.
- Do not bypass BIR/prepared BIR, AArch64 target MIR, or machine-node selection
  when serving the public CLI route.
- Do not implement an internal assembler, encoder, object writer, linker, or
  `.s` parser in this plan.
- Keep smoke proof rooted in the public `c4cll` route and the external
  toolchain when available.
- Distinguish record-level constructor coverage from the source-to-module
  pipeline: `aarch64::module::build(...)` currently appends only spill/reload
  machine nodes.
- Scalar records currently do not carry a destination register operand, and
  load records currently do not carry a destination register operand; the
  printer intentionally rejects those instead of guessing from value ids.
- The public AArch64 route is wired, but ordinary source cases currently
  produce no selected printable machine nodes because
  `aarch64::module::build(...)` only appends spill/reload machine nodes.
- Step 4 needs a real selected machine-node-producing source path before it can
  validate printed `.s` with an external AArch64 assembler; do not satisfy that
  smoke by parsing fixture assembly or bypassing prepared/module selection.

## Proof

Delegated proof passed and was written to `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_cli|backend_codegen|backend_aarch64_prepared_frame_control|backend_aarch64_target_instruction_records)') > test_after.log 2>&1`

Result: 111 tests matched; 99 ran and passed, with 12 existing x86 route-debug
tests disabled by CTest properties. The focused new
`backend_cli_aarch64_asm_no_machine_nodes_fails` case passed and proves the
public AArch64 asm command reaches the printer boundary and fails without
writing fallback output when no selected printable machine nodes exist.
