Status: Active
Source Idea Path: ideas/open/128_aarch64_wide_value_owner_post_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit f128 Helper Resources, Transport, And Printing

# Current Packet

## Just Finished

Step 5: Audit f128 Helper Resources, Transport, And Printing completed the
read-only classification of f128 helper resource policy, preservation checks,
transport construction, printable-address handling, Q-register spelling, and
printer-facing record fields against `f128.cpp`, `instruction.cpp/.hpp`,
`machine_printer.cpp`, and helper-policy sources.

Classification:

- f128 helper resource, ABI, clobber, preservation, and selected-call policy:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `populate_f128_runtime_helper_boundary_policy` creates the resource policy,
  initial ABI policy, live-preservation policy, and clobber set in prealloc
  (prealloc/f128_runtime_helpers.cpp lines 882-915). Prealloc then derives ABI
  bindings (lines 628-724), marshaling moves (lines 762-880), live-preservation
  facts through `build_call_preserved_values` (lines 917-960), and selected-call
  ownership booleans (lines 963-1051). The public helper predicates
  `prepared_f128_runtime_helper_has_scalar_cmp_result_bridge_contract` and
  `prepared_f128_runtime_helper_has_abi_contract` encode arithmetic,
  comparison, and f128/scalar cast ABI contracts (lines 1055-1154).
- f128 helper-boundary record construction:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `make_prepared_f128_runtime_helper_boundary_record` rejects missing helper
  facts, unsupported source operations, unsupported result ownership, missing
  resource policy, missing prepared ABI policy, missing clobber policy, and
  incomplete live-preservation or selected-call ownership before it creates a
  `F128RuntimeHelperBoundaryRecord` (f128.cpp lines 1989-2114). The record then
  copies prepared policy fields, clobbers, value ids/names, source/result types,
  helper family/kind, callee identity, marshaling records, and `source_helper`
  provenance (lines 2115-2263). This is a target machine payload over prepared
  policy, not late helper-policy selection.
- f128 helper operand/scalar conversion:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `make_f128_helper_operand_record` requires prepared carrier binding, ABI
  binding, marshaling move, matching value ids/names, full-width Vreg/vector
  16-byte carrier facts, a matching prepared f128 carrier, and successful
  AArch64 register conversion before accepting the operand (f128.cpp lines
  1865-1946). `make_f128_helper_scalar_record` similarly consumes prepared
  scalar ownership, ABI binding, and scalar marshaling facts for FPR 4/8-byte
  scalar casts (lines 1949-1987). `make_f128_abi_register_operand`,
  `make_f128_scalar_register_operand`, and
  `make_f128_cmp_materialized_i1_register_operand` choose only the AArch64 view
  used to spell prepared bindings as Q/W/S/D registers (lines 1686-1779).
- f128 helper preservation/source-policy validation:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption`.
  `has_complete_f128_live_preservation`,
  `has_complete_f128_selected_call_ownership`, and
  `has_complete_f128_helper_resource_policy` verify the presence of prepared
  policy fields (f128.cpp lines 516-570). The source-policy matcher then checks
  that the machine record still matches the exact `PreparedF128RuntimeHelper`
  provenance, including resource policy, ABI policy, live preservation,
  selected-call ownership, clobbered registers, carrier bindings, ABI bindings,
  marshaling moves, scalar ownership, and comparison-result consumption (lines
  626-1009). This is preservation/provenance validation, not a new preservation
  authority.
- f128 transport and printable-address handling:
  `shared-bir-prealloc-contract` consumed by `aarch64-codegen-consumption` for
  facts; `local-organization-only` for address spelling. `F128TransportRecord`
  holds prepared value id/name, carrier kind, size/alignment, register
  placement, slot id, stack offset, optional memory operand, and source-carrier
  provenance (instruction.hpp lines 1084-1107). `f128_printable_memory_address`
  either prints an already encodable frame-slot/direct memory operand or
  materializes a frame-slot/symbol address with reserved AArch64 scratch
  registers (f128.cpp lines 319-360). This is printer address materialization
  over existing `MemoryOperand` facts; it does not select memory-backed carrier
  authority or stack homes.
- f128 Q/vector/scalar register spelling:
  `local-organization-only` and `aarch64-codegen-consumption`.
  `f128_q_register_name` only accepts Q-view Vreg/vector contiguous-width-1
  operands and `f128_vector_register_name` spells the same prepared register as
  `vN.16b` for helper `mov` instructions (f128.cpp lines 171-179 and
  1376-1388). Scalar helper printing uses W/S/D views through
  `f128_register_name_with_view` and `f128_scalar_fp_register_name` (lines
  147-168 and 388-409). These are AArch64 spelling details and should stay
  target-local.
- f128 printer-facing record and machine effects:
  `aarch64-codegen-consumption`. `make_f128_transport_instruction` builds
  operands, defs/uses, memory side effects, selection status, and an
  `F128TransportRecord` payload (f128.cpp lines 2266-2323).
  `make_f128_runtime_helper_boundary_instruction` builds operands, defs/uses,
  clobbers from prepared call clobbers, call side effects, and a
  `F128RuntimeHelperBoundaryRecord` payload (lines 2326-2381). The global
  machine printer simply dispatches `F128TransportRecord` and
  `F128RuntimeHelperBoundaryRecord` payloads to `print_f128_transport` and
  `print_f128_runtime_helper` (machine_printer.cpp lines 2106-2165).
- f128 helper invocation printing:
  `aarch64-codegen-consumption` and `local-organization-only`.
  `print_f128_runtime_helper` first revalidates prepared source policy, then
  prints marshaling `mov`/`fmov` lines, `bl <callee>`, comparison `cmp`/`cset`,
  and scalar/f128 cast moves according to the prepared helper family and ABI
  transition (f128.cpp lines 1486-1640). `print_f128_transport` prints comments
  for carrier snapshots and `ldr`/`str` sequences for full-width or
  memory-backed f128 transports using printable memory addresses and scratch Q
  registers (lines 1391-1484). These are final AArch64 assembly details.

Gap candidates:

- No Step 5 shared-policy gap is justified. Helper resource policy, ABI
  contract, clobber policy, live preservation, selected-call ownership,
  carrier/ABI bindings, scalar bridge ownership, and marshaling moves are all
  prepared before AArch64 lowering and then cross-checked in `f128.cpp`.
- No precise follow-up idea is warranted from Step 5. A valid future candidate
  would need to show AArch64 inventing helper ABI/resource/preservation policy,
  selected-call ownership, carrier binding, scalar comparison consumption, or
  stack/publication authority without prepared input. Current evidence shows
  AArch64 rejects missing or inconsistent prepared facts and limits itself to
  machine-record construction plus printable AArch64 assembly.

## Suggested Next

Execute Step 6 from `plan.md`: synthesize the closure payload from the completed
i128 and f128 audit evidence without starting implementation work.

## Watchouts

- This route is analysis-only; do not edit implementation files, tests, or
  build metadata.
- Do not treat `i128_ops.cpp` or `f128.cpp` size as evidence of a boundary gap.
- Do not move AArch64 register spelling, Q-register spelling, lane/shift
  opcode spelling, or helper call assembly into shared BIR/prealloc code.
- Follow-up ideas must be concrete: owner boundary, filenames, proof route, and
  reject signals.
- Step 5 found no shared-policy gap for f128 helper resources, preservation,
  transport construction, printable addresses, Q-register spelling, or
  printer-facing records.
- Keep printable-address materialization, Q/vector/scalar register spelling,
  `mov`/`fmov`/`ldr`/`str`/`bl` emission, machine-effect construction, and
  payload dispatch target-local.
- Closure synthesis should separate prepared helper policy in
  `src/backend/prealloc/f128_runtime_helpers.cpp` from AArch64 consumption in
  `src/backend/mir/aarch64/codegen/f128.cpp`; the apparent duplication in
  `f128.cpp` is provenance/completeness cross-checking.

## Proof

Proof: analysis-only, no build/test run.
