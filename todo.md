Status: Active
Source Idea Path: ideas/open/95_aarch64_calls_immediate_scalar_argument_publication_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Immediate Publication Boundary And Proof Subset

# Current Packet

## Just Finished

Completed plan Step 1: `Confirm Immediate Publication Boundary And Proof Subset`.

Mapped the immediate scalar call-argument publication cluster in
`src/backend/mir/aarch64/codegen/calls.cpp`:

- Same-block lookup: `find_same_block_cast_producer` finds a same-block
  `bir::CastInst` for an already prepared source home. It should stay a lookup
  consumed by the owner, not a new scalar producer selector.
- Immediate bit extraction: `integer_width_bits_for_type` and
  `immediate_integer_bits` are immediate-publication helpers for supported
  integer widths. Keep them inside or directly private to the owner.
- Destination view rendering: the `scalar_gp_register_view` overload and the
  `scalar_fp_register_view(abi::RegisterReference, bir::TypeKind)` overload
  are used by immediate cast publication. Wrap or move them narrowly. The
  `scalar_fp_register_view(bir::TypeKind)` overload is also used by
  `call_boundary_move_selection_status` and
  `lower_before_call_immediate_binding`, so keep that type-only view outside
  or wrap it without absorbing ordinary before-call authority.
- FP immediate materialization: `append_materialize_fp_immediate` belongs to
  the immediate publication owner because it emits the scratch GP materialize
  plus `fmov` sequence used only by immediate cast publication.
- Publication line construction:
  `make_immediate_cast_call_argument_publication_lines` is the main owner
  body. It owns supported immediate cast materialization for `FPToSI`,
  `FPToUI`, `SIToFP`, `UIToFP`, `FPExt`, and `FPTrunc`, including GP/FP
  scratch selection, register view rendering, integer bit extraction, and
  inline-asm line spelling.
- Instruction construction:
  `make_immediate_cast_call_argument_publication_instruction` should become
  the owner entry point or a tiny owner wrapper. It may consume
  `BlockLoweringContext`, `PreparedValueHome`, destination register operand,
  and instruction index to preserve existing record fields and effects.
- Before-call integration: `lower_before_call_immediate_binding` remains
  outside the owner. It owns prepared immediate fact lookup, diagnostic checks,
  stack-slot immediate stores, prepared destination register authority,
  synthetic move construction, and ordinary `CallBoundaryMove` creation.

Smallest proposed boundary: introduce a local `ImmediateScalarCallArgumentPublicationOwner`
in `calls.cpp` that owns only same-block immediate cast publication into a
call ABI register. Its narrow public method should produce the existing
`MachineInstruction` for an already accepted prepared source home and prepared
destination register, while private methods own cast lookup consumption,
integer bit extraction, GP/FP register views, FP immediate materialization,
inline-asm line construction, and assembler instruction construction.

## Suggested Next

Execute Step 2 by introducing the local
`ImmediateScalarCallArgumentPublicationOwner` in `calls.cpp` and routing only
`make_immediate_cast_call_argument_publication_instruction` plus its private
line/materialization helpers through that owner. Leave
`lower_before_call_immediate_binding` as the prepared immediate consumer.

## Watchouts

- Keep this route limited to AArch64 calls-side immediate scalar call argument
  publication.
- Do not move publication-source authority, prepared call argument plans,
  source-home selection, scalar producer selection, aggregate byval transport,
  f128 carrier handling, ordinary before-call moves, or call-boundary record
  ownership into the new owner.
- Preserve supported immediate forms, unsupported diagnostics, inline-asm
  spelling, record fields, selection status, and assembly output.
- Keep `lower_before_call_immediate_binding` as the consumer of prepared
  immediate argument facts.
- Do not move `find_immediate_argument_in_call_plan`,
  `prepare::find_indexed_prepared_immediate_call_argument`,
  `make_scalar_call_argument_immediate`, `scalar_integer_register_view`,
  `scalar_integer_type_from_size`, `scalar_size_from_register_view`,
  `make_register_operand_from_prepared_authority`,
  `make_call_boundary_move_instruction`, `make_memory_instruction`, or generic
  `materialize_integer_constant_lines` into the owner.
- Keep call-boundary record construction local to the owner only for the
  immediate cast assembler publication. Ordinary before-call moves, stack
  immediate stores, aggregate byval lanes, f128 carriers, after-call result
  publication, and general constant materialization remain outside.
- The same-block cast lookup can be a private helper under the owner, but it
  must not become authority to search non-local producers or rederive source
  selection when prepared source-home facts are absent.

## Proof

Audit-only packet; no build/tests run because no implementation files were
touched.

Post-implementation focused proof command:

```bash
cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$' --output-on-failure > test_after.log
```

Coverage intent: `backend_aarch64_instruction_dispatch` covers same-block
immediate cast publication, GP/FP destination views, integer and FP immediate
cast inline-asm spelling, and immediate stack call arguments;
`backend_aarch64_return_lowering` covers ordinary scalar integer and FP
immediate before-call moves; the two CLI tests cover immediate-fold/prepared
AArch64 publication contracts; `backend_aarch64_call_boundary_owner` and
`backend_aarch64_target_instruction_records` cover nearby call-boundary and
record surfaces; the scalar FP route test keeps nearby FP publication behavior
in scope.
