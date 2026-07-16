Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Delete semantic string escape hatches

# Current Packet

## Just Finished

Completed Step 3 repair for the rejected hook full-suite baseline candidate
after commit `fa7e3261c Drop signature type text equality checks`.
`test_baseline.new.log` had one new failure,
`frontend_lir_function_signature_type_ref`, because signature-store comparison
accepted a stale rendered aggregate mirror when native `StructNameId` identity
still matched.

Restored signature-store mirror equality only at the output-boundary comparison
sites: `same_signature_store_type_fact(...)` in
`src/codegen/lir/verify.cpp` and the local `same_type` lambda inside
`LirModule::same_function_signature_entry(...)` in
`src/codegen/lir/ir.hpp` now require both native `LirTypeRef` equality and
matching rendered text. This keeps stale function signature mirrors rejected
without restoring general verifier-side text reparsing.

## Suggested Next

Supervisor should commit this repair slice if accepted, then rerun or resume the
baseline path that rejected `test_baseline.new.log`. Do not continue to the next
Step 3 candidate until the repaired baseline is accepted.

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
- `LirTypeRef::operator==` already carries native facts for array shape,
  anonymous aggregate layout, named aggregate identity/kind, and text fallback
  for unstructured refs. This packet did not need to strengthen it.
- `rg -n "lhs == rhs && lhs\.str\(\) == rhs\.str\(\)|a == b && a\.str\(\) == b\.str\(\)" src/codegen/lir/verify.cpp src/codegen/lir/ir.hpp`
  is now clean.
- `src/codegen/lir/verify.cpp` no longer has the selected
  `type_ref_mismatch_detail(...)` reparses from `LirTypeRef(type.str())` for
  kind, integer width, or VRM width.
- Signature-store mirror text equality is intentionally preserved at the
  function signature store boundary; it is output-boundary consistency, not
  semantic type authority.
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
'^frontend_lir_function_signature_type_ref$|^frontend_lir_call_type_ref$'
--output-on-failure > test_after.log
2>&1`.
`test_after.log` contains the focused CTest subset output with 2/2 tests
passing, including the previously failing
`frontend_lir_function_signature_type_ref`.

The clean-search done condition remains clean:
`rg -n "LirTypeRef\(type\.str\(\)\)\.(kind|integer_bit_width|vrm_width)" src/codegen/lir/verify.cpp`
returned no matches.
The supervisor-selected proof was sufficient for this packet.
