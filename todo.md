Status: Active
Source Idea Path: ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Stage 1 Inline Asm Surfaces

# Current Packet

## Just Finished

Step 1: Map Stage 1 Inline Asm Surfaces completed as read-only discovery.

Concrete Stage 1 surfaces Stage 2 should extend:

- Constraint classification and metadata creation:
  `src/backend/bir/lir_to_bir/calling.cpp`
  `split_inline_asm_constraint_tokens`, `make_inline_asm_metadata`, and
  `inline_asm_template_modifier_facts`. Today `r`, `=r`, numeric tied inputs,
  `i`/`I`, `m`, `p`, and clobbers become `bir::InlineAsmOperandMetadata`;
  unsupported strings produce `unsupported_constraintN:<token>`.
- BIR metadata carrier:
  `src/backend/bir/bir.hpp` `InlineAsmOperandKind`,
  `InlineAsmOperandMetadata`, and `InlineAsmMetadata`; dump visibility is in
  `src/backend/bir/bir_printer.cpp`.
- Prepared inline asm carrier validation:
  `src/backend/prealloc/inline_asm.cpp`
  `rv64_inline_asm_register_identity`,
  `inline_asm_identity_matches_register_constraint`,
  `populate_inline_asm_home_identity`,
  `make_prepared_inline_asm_operand`,
  `validate_inline_asm_carrier`, `build_inline_asm_carrier`, and
  `populate_inline_asm_carriers`. This is where scalar RV64 homes are proven as
  GPR identities and tied scalar operands require shared concrete homes.
- Register class and group allocation machinery:
  `src/backend/prealloc/regalloc.hpp` `PreparedRegisterClass::Vector`,
  `PreparedRegisterGroupOverride`, `PreparedAllocationConstraint`, and
  `PreparedPhysicalRegisterAssignment`; `src/backend/prealloc/module.hpp`
  `PreparedBirModule::register_group_overrides`; and
  `src/backend/prealloc/regalloc/classification.cpp`
  `resolve_register_class` / `resolve_register_group_width`.
- RV64 vector register span allocation:
  `src/backend/prealloc/target_register_profile.cpp`
  `riscv_caller_saved_pool`, `riscv_callee_saved_pool`,
  `contiguous_group_spans`, and `register_spans_for_pool`. Current RV64 vector
  spans are generated over `v0..v15` and `v16..v31`, with grouped bases stepping
  by `contiguous_width`; Stage 2 must confirm this matches the intended
  `v0..v31` allocation contract and any caller/callee split policy.
- Regalloc publication and overlap behavior:
  `src/backend/prealloc/regalloc.cpp` `BirPreAlloc::run_regalloc`,
  especially `caller_saved_register_spans` / `callee_saved_register_spans`,
  active assignment `occupied_register_names`, interference publication, and
  stack fallback.
- RISC-V substitution helper surface:
  `src/backend/mir/riscv/codegen/asm_emitter.cpp`
  `classify_rv_constraint`, `format_riscv_operand`, and
  `substitute_riscv_asm_operands`; legacy standalone mirror exists in
  `src/backend/mir/riscv/codegen/inline_asm.cpp`.
- RV64 direct object path:
  `src/backend/mir/riscv/codegen/object_emission.cpp`
  `rv64_register_number_for_inline_asm_operand`,
  `fragment_for_rv64_insn_r_inline_asm`,
  `fragment_for_prepared_call`, `find_prepared_inline_asm_carrier`,
  `prepared_function_to_object_function`, and
  `build_rv64_prepared_text_object_module`. Stage 1 object proof encodes
  scalar `.insn r` from complete prepared carriers and rejects raw registers,
  named operands, template modifiers, clobbers, malformed `.insn`, incomplete
  carriers, and unsupported constraints.
- Stage 1 proof fixtures:
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
  `make_structured_inline_asm_metadata_module` and
  `inline_asm_lir_lowering_preserves_structured_operand_metadata`;
  `tests/backend/bir/backend_prepared_printer_test.cpp`
  `prepare_rv64_inline_asm_scalar_carrier_module` and
  `rv64_inline_asm_carriers_preserve_scalar_register_identities`;
  `tests/backend/bir/backend_prepare_liveness_test.cpp`
  `prepare_vector_grouped_linear_scan_module_with_regalloc`,
  `check_vector_grouped_linear_scan`, and
  `check_vector_grouped_cross_call`; and
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`
  `make_prepared_inline_asm_insn_r_module`,
  `builds_prepared_inline_asm_insn_r_object`,
  `builds_prepared_inline_asm_insn_r_tied_input_object`, plus the
  `rejects_prepared_inline_asm_insn_r_*` negative cases.

Source-language value representation limitation found: BIR `TypeKind` has no
native vector value type (`Void`, integer widths, `Ptr`, and float widths
only). Existing vector allocation tests drive `PreparedRegisterClass::Vector`
through `PreparedRegisterGroupOverride` on ordinary named values, so Stage 2
should first bridge `VR`/`VRM2`/`VRM4` inline asm constraints into backend
metadata/prepare overrides rather than broadening frontend value representation.

## Suggested Next

Delegate Step 2 to add vector constraint classification and carrier facts:
extend BIR inline asm operand metadata enough to distinguish scalar `r` from
`VR`/`VRM2`/`VRM4`, publish prepared register-group overrides for the tied
input/output values, and add focused BIR/prepared-printer tests before touching
object encoding.

## Watchouts

- Do not reactivate the parked Stage 1 child unless lifecycle rules explicitly
  require it.
- Keep Stage 2 focused on `VR`, `VRM2`, and `VRM4`; EV `.insn.d`,
  named-operands, masks, and consteval asm strings are later children.
- Treat testcase-overfit, expectation weakening, and raw-string constraint
  acceptance as route failures.
- Current `InlineAsmOperandKind` only says register input/output/tied, not
  target register class or group width. Avoid accepting `VR*` as opaque
  supported strings without carrying class/width into prepare/regalloc.
- Current tied validation assumes GPR-compatible identities through
  `inline_asm_identity_matches_register_constraint`; vector tied operands need
  equivalent vector identity/group compatibility.
- Current RV64 object helper
  `rv64_register_number_for_inline_asm_operand` hard-codes `=r`, `+r`, `r`,
  and numeric tied GPR operands. Vector substitution should not weaken these
  scalar fail-closed checks.
- The existing RV64 vector span helper splits caller/callee pools into
  `v0..v15` and `v16..v31`; the Stage 2 contract says allocation is over
  `v0..v31`, so the implementation packet should decide whether this policy is
  acceptable or needs a target-profile adjustment.

## Proof

Discovery-only packet. No build, CTest, or root-level proof log was required or
created.
