Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Delete mutable LIR type text escape hatches in small packets

# Current Packet

## Just Finished

Completed Step 2 packet to delete the indexed-GEP element type-text
compatibility helper family. Removed
`hir_rendered_indexed_gep_element_type_text` from
`src/codegen/lir/hir_to_lir/lvalue.cpp` and replaced its direct fallback
callsite with explicit `LirTypeRef(...)` construction around the same rendered
element type text.

## Suggested Next

Continue Step 2 with the obvious remaining named compatibility helper family
`direct_owned_aggregate_type_text` in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, if the supervisor selects another
small helper-deletion packet. That file was explicitly outside this packet.

## Watchouts

- Do not restore a mutable `LirTypeRef::str()` accessor or add a test-only
  backdoor.
- Do not restore mutable `operator std::string&()` or replace it with another
  mutable text escape hatch.
- Do not restore const `LirTypeRef` implicit text conversions; use `.str()` or
  typed/native access at direct users.
- Do not add a renamed generic runtime-text factory; remaining text-backed
  constructions should stay behind named compatibility factories or explicit
  constructors until their own packet deletes them.
- Do not delete const `str()`, additional named compatibility factories,
  non-`LirTypeRef` wrapper conversions, or equality/classification helpers in
  the same packet.
- `rg -n "hir_rendered_aggregate_field_signature_type_text" src tests/frontend tests/backend`
  is now clean.
- `rg -n "hir_inline_asm_type_text" src tests/frontend tests/backend` is now
  clean.
- `rg -n "parsed_typed_call_return_text" src tests/frontend tests/backend`
  is now clean.
- `rg -n "parsed_typed_call_argument_text" src tests/frontend tests/backend`
  is now clean.
- `rg -n "no_module_va_list_tag_type_text" src tests/frontend tests/backend`
  is now clean.
- `rg -n "no_module_amd64_va_list_tag_type_text" src tests/frontend tests/backend`
  is now clean.
- `rg -n "hir_rendered_aarch64_vector_abi_source_type_text|hir_rendered_aarch64_vector_call_argument_abi_source_type_text" src tests/frontend tests/backend`
  is now clean.
- `rg -n "hir_rendered_call_target_type_text" src tests/frontend tests/backend`
  is now clean.
- `rg -n "hir_rendered_indexed_gep_element_type_text" src tests/frontend tests/backend`
  is now clean.
- Required scalar-to-vector splat shuffles now reject incoherent native
  `mask_type` mirrors against vector-store lane count; do not weaken that
  baseline repair.
- Remaining `.str() =` lines in the unowned
  `tests/frontend/frontend_lir_call_type_ref_test.cpp` are `LirOperand`
  presentation mutations, not `LirTypeRef` mutations.
- `rg -n "LirTypeRef::runtime_text" src tests/frontend tests/backend` is now
  clean.
- `rg -n "operator const std::string&\(\) const|operator std::string_view\(\) const" src/codegen/lir/types.hpp`
  still reports `LirBinaryOpcodeRef` and `LirCmpPredicateRef`; those conversions
  are explicitly outside this packet and should not be treated as `LirTypeRef`
  matches.

## Proof

Proof run passed:
`cmake --build build && ctest --test-dir build -R
'^frontend_lir_call_type_ref$'
--output-on-failure > test_after.log
2>&1`.
`test_after.log` contains the focused CTest subset output with 1/1 test
passing. The supervisor-selected proof was sufficient for this packet.
