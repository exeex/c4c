Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Delete semantic string escape hatches

# Current Packet

## Just Finished

Completed Step 2 packet to delete the remaining named compatibility helper
family `direct_owned_aggregate_type_text` from
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`. The signature return and parameter
call sites now pass their rendered signature text directly through the existing
`lir_signature_type_ref(...)` native aggregate type-ref construction path.

## Suggested Next

Step 2 is exhausted for current evidence: no further obvious bounded
Step 2-only named `LirTypeRef` compatibility helper deletion remains after the
accepted helper-deletion packets through commit `4ff1f0dc4`.

Start Step 3 with a narrow verifier-side semantic string authority packet in
`src/codegen/lir/verify.cpp`: remove the textual reparsing authority in
`type_ref_mismatch_detail(...)` where `LirTypeRef(type.str()).kind()`,
`LirTypeRef(type.str()).integer_bit_width()`, and
`LirTypeRef(type.str()).vrm_width()` rederive semantic facts from rendered text.
Replace those checks with typed/native `LirTypeRef` facts or delete them where
the typed facts are already authoritative. Keep signature-text compatibility
parsing, byval ABI fragment checks, and aggregate signature mirror validation
out of this first packet unless the compile/build proof forces a directly
related local adjustment.

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
- Step 3 owns verifier-side textual reparsing and textual
  equality/classification authority. Do not mix it with Step 4 adapter removal.
- `src/codegen/lir/verify.cpp` still has semantic text reparsing in
  `type_ref_mismatch_detail(...)` at the `LirTypeRef(type.str())` checks; that
  is the selected next packet.
- `function_signature_line(...)` parsing of `signature_text`,
  `aggregate_signature_param_mirror_matches_type(...)` byval fragment checks,
  and direct aggregate signature mirror checks are compatibility/output
  validation surfaces; leave them alone unless separately selected.
- `rg -n "direct_owned_aggregate_type_text" src tests/frontend tests/backend`
  is now clean.
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
passing. The clean-search done condition also passed:
`rg -n "direct_owned_aggregate_type_text" src tests/frontend tests/backend`.
The supervisor-selected proof was sufficient for this packet.
