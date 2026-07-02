Status: Active
Source Idea Path: ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Scalar Helper Ownership

# Current Packet

## Just Finished

Step 1: mapped scalar helper ownership for the RV64 object scalar fragment
cleanup without implementation edits.

Move set for Step 2:

- Move the object-fragment scalar binary wrapper cluster into
  `prepared_scalar_emit.*`: the forward declaration and definition of
  `fragment_for_prepared_binary`, keeping it callable from
  `fragment_for_prepared_instruction`, select-edge source producer/dependency
  paths, and unsupported-instruction diagnostics.
- Move the scalar cast fragment cluster into `prepared_scalar_emit.*`:
  `fragment_for_prepared_cast`, `fragment_for_prepared_floating_cast`,
  `fragment_for_prepared_fp_to_int_cast`, `fragment_for_prepared_int_to_fp_cast`,
  `fragment_for_prepared_pointer_cast`,
  `rv64_prepared_pointer_cast_types_supported`,
  `append_rv64_zero_extend_register`, and
  `append_rv64_sign_extend_register`.
- Move scalar compare branch fragment construction into
  `prepared_scalar_emit.*`: `fragment_for_prepared_compare_branch`, while
  preserving compare predicate normalization and branch-label fixup emission.
- Move scalar compare-value trunc publication into `prepared_scalar_emit.*`:
  `prepared_compare_feeds_supported_scalar_trunc_publication` and
  `fragment_for_prepared_scalar_compare_trunc_source`, while keeping its
  diagnostic caller wired.
- Move simple scalar return fragment construction into `prepared_scalar_emit.*`:
  `fragment_for_prepared_return`, including scalar/FPR immediate returns,
  before-return already-loaded checks, direct global pointer return authority,
  and epilogue/ret emission. Do not move before-return move bundle discovery or
  full terminator dispatch.

Parked set:

- Leave `fragment_for_prepared_instruction` and
  `fragment_for_prepared_terminator` in `object_emission.cpp`; they are broad
  dispatch/fallback owners, not scalar helper ownership.
- Leave select-edge publication ownership in `object_emission.cpp`, including
  `fragment_for_prepared_select_edge_source_producer`,
  `fragment_for_prepared_select_edge_source_dependencies`,
  `fragment_for_prepared_select_edge_binary_with_cast_dependencies`,
  `prepared_binary_is_select_edge_owned_source`,
  `prepared_cast_is_available_select_edge_dependency_authority_source`, and
  related source-producer suppression/authority checks. These may call moved
  scalar APIs but should not move in this cleanup.
- Leave ordinary select lowering parked for this runbook:
  `fragment_for_prepared_select` and its same-block select producer helpers are
  select-owned despite living near scalar helpers.
- Leave pointer-specific fused branch helpers parked:
  `prepared_pointer_branch_operand_home_for`,
  `prepared_pointer_branch_predicate_is_supported`,
  `normalize_prepared_pointer_branch_predicate`,
  `prepared_branch_condition_is_supported_pointer_branch`, and
  `fragment_for_prepared_fused_pointer_branch`.
- Leave call, memory, object-data, variadic, inline-asm, frame-address, symbol
  address, and function traversal helpers parked in their current owners.
- Leave public legacy helpers in `object_emission.hpp` parked:
  `make_rv64_return_zero_fragment`, `make_rv64_direct_call_fragment`, and
  `make_rv64_pcrel_address_fragment`.

Include/declaration prerequisites for Step 2:

- Add `RiscvEncodedFragment`-returning declarations for the moved scalar object
  fragment helpers to `prepared_scalar_emit.hpp`; include or forward-declare the
  required BIR/prepared types consistently with existing extracted modules.
- Let `object_emission.cpp` include `prepared_scalar_emit.hpp` and call moved
  helpers through that header; remove only local forward declarations that are
  replaced by public scalar declarations.
- `prepared_scalar_emit.cpp` will need object-fragment types from
  `object_emission.hpp`, context from `prepared_emit_context.hpp`, frame/store
  helpers from `prepared_frame_emit.hpp`, instruction encoders and
  `rv64_append_le32` from `rv64_line_assembler.hpp`, and contract/publication
  helpers already used by the scalar string emitters.
- Prefer existing prepared-frame helper names where behavior matches:
  `append_rv64_prepared_move`, `append_rv64_prepared_load_immediate`, and
  `append_rv64_prepared_store_register_to_stack_offset`. Keep any remaining
  branch-label/fixup helpers private to scalar or explicitly declared rather
  than exporting object-wide append internals.
- `append_rv64_move_value_to_register` is the main shared prerequisite. It is
  currently called by binary, call, return, int-to-FP cast, cast, select-edge
  binary-with-cast-dependencies, select, compare branch, and fused pointer
  branch paths. Step 2 should either move it as a scalar API with a declaration
  visible to the still-parked callers, or keep a compatibility wrapper in
  `object_emission.cpp` that forwards to the scalar implementation.

Candidate dead wrappers for Step 3 proof:

- After Step 2 moves the implementations and rewires callers,
  caller-proof the old local definitions/declarations for
  `fragment_for_prepared_binary`, `fragment_for_prepared_cast`,
  `fragment_for_prepared_compare_branch`,
  `fragment_for_prepared_scalar_compare_trunc_source`, and
  `fragment_for_prepared_return`; delete only if no local wrapper boundary is
  still needed.
- After Step 2 moves the cast cluster, caller-proof old local helpers
  `fragment_for_prepared_floating_cast`,
  `fragment_for_prepared_fp_to_int_cast`,
  `fragment_for_prepared_int_to_fp_cast`,
  `fragment_for_prepared_pointer_cast`,
  `rv64_prepared_pointer_cast_types_supported`,
  `append_rv64_zero_extend_register`, and
  `append_rv64_sign_extend_register`.
- Do not mark `fragment_for_prepared_binary` itself dead unless the moved
  scalar declaration remains the only implementation; AST caller checks show it
  is still used by select-edge producer/dependency paths, instruction dispatch,
  and diagnostics.
- Do not mark `append_rv64_move_value_to_register` dead after Step 2 unless all
  object-side call sites are intentionally rewired; AST caller checks show
  live users across call, return, cast, select, compare branch, fused pointer
  branch, and select-edge publication paths.

## Suggested Next

Execute Step 2 from `plan.md`: extract the mapped scalar fragment helpers into
`prepared_scalar_emit.*`, add the declarations/includes above, keep parked
select/publication/dispatch helpers in `object_emission.cpp`, and preserve
behavior.

## Watchouts

- Keep select-edge publication, predecessor publication movement, broad
  dispatch, and `fragment_for_prepared_instruction` out of this cleanup.
- Preserve compare normalization, branch-label fixups, diagnostics, emitted
  bytes, tests, expectations, and unsupported markers.
- Do not touch `review/global_address_helper_cleanup_review.md`.
- AST caller evidence gathered in this packet:
  `fragment_for_prepared_binary` has direct callers in select-edge source
  producer/dependency helpers, `fragment_for_prepared_instruction`, and
  diagnostics; `fragment_for_prepared_cast` has a direct caller in
  `fragment_for_prepared_instruction`; `fragment_for_prepared_return` and
  `fragment_for_prepared_compare_branch` have direct callers in
  `fragment_for_prepared_terminator`; `fragment_for_prepared_select` has a
  direct caller in `fragment_for_prepared_instruction`.
- The scalar compare-value string emitter already lives in
  `prepared_scalar_emit.cpp`; the object-fragment compare/trunc publication
  wrapper is the piece mapped for movement.

## Proof

No build run required for this mapping-only packet; no `test_after.log` was
written.

Exact Step 2 validation command from `plan.md`:

```bash
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_prepared_fused_compare|codegen_route_riscv64_prepared_fused_compare|obj_runtime_rv64_return_add|obj_runtime_rv64_return_add_sub_chain)'" > test_after.log 2>&1
```
