Status: Active
Source Idea Path: ideas/open/241_inline_asm_clobber_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select AArch64 Clobber Facts

# Current Packet

## Just Finished

Plan Step 3 - Select AArch64 Clobber Facts.

Added selected AArch64 inline-asm clobber carriage from prepared structured
clobber facts:

- selected assembler records now carry `PreparedInlineAsmCarrier::clobbers`
  into `AssemblerInstructionRecord::inline_asm_clobbers`
- supported structured clobbers are validated before selection and lowered to
  machine clobber effects for GPR registers, `memory`, and `cc`
- malformed, unknown, and target-invalid clobber facts fail closed during
  AArch64 dispatch with explicit diagnostic facts
- machine printing accepts the selected clobber list without parsing template
  text, final assembly text, or rendered constraint strings

## Suggested Next

Plan Step 4 packet: decide the next inline-asm authority boundary from the
active plan, keeping memory/address constraints and tied-home allocation policy
in their separate open ideas unless the supervisor switches lifecycle state.

## Watchouts

- Do not parse clobbers from rendered template text, final assembly, or
  diagnostic strings.
- Treat source parser clobber children as the authority; rendered `~{...}`
  constraint text is compatibility spelling, not ownership.
- This slice supports selected AArch64 GPR clobbers plus `memory` and `cc`;
  SIMD/FP clobber spellings are still fail-closed as unknown.
- Preserve the completed inline-asm behavior from closed idea 240.
- Keep memory/address constraints in
  `ideas/open/242_inline_asm_memory_address_constraints.md`.
- Keep alias-aware tied-home allocation policy in
  `ideas/open/243_inline_asm_tied_home_allocation_policy.md`.

## Proof

Delegated proof passed:

`cmake --build --preset default --target backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test`

`ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$' | tee test_after.log`

Proof log: `test_after.log`.
