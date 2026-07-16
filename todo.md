Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed Step 3: semantic string escape-hatch packets have been accepted and
committed through the operand, comparison, arithmetic, call argument, and
ternary preservation slices. Mutable `str()` accessors, implicit LIR operand
string conversions, and selected frontend display-text equality gates are no
longer used as valid-LIR semantic authority.

Remaining `.str()` checks are classified as verifier/signature/call formatting
mirror or output-boundary consistency surfaces, not selected mutable,
implicit-conversion, or semantic preservation gates.

## Suggested Next

Begin Step 4 with a narrow adapter/fallback audit packet: inspect receiver,
dispatcher, verifier, printer, construction, and HIR-to-LIR boundary paths for
expired M1--M15 adapters or universal fallbacks that can still re-enter deleted
compatibility. Record each candidate as removable, already absent, or preserved
with an explicit out-of-scope owner before making code changes.

## Watchouts

- Step 4 should remove expired adapters and universal fallbacks only after the
  audit identifies their accepted M1--M15 replacement evidence.
- Resolve compile errors with native-specific facts, not new compatibility
  shims, generic runtime-text factories, or renamed universal bags.
- Preserve residual owner-specific strings by documenting their owner in this
  file; do not silently move 812/813 non-type strings, 821/822 switch selector
  surfaces, or 797 coverage convergence into this deletion packet.
- Do not restore mutable `LirTypeRef::str()`, mutable `LirOperand::str()`, or
  implicit LIR string conversions while removing adapters.
- Signature-store mirror text equality is intentionally preserved at the
  function signature store boundary; it is output-boundary consistency, not
  semantic type authority.
- `same_signature_store_type_fact(...)` in `src/codegen/lir/verify.cpp` and
  `LirModule::same_function_signature_entry(...)` behavior were left intact.
- The remaining `lhs.str() == rhs.str()` search hit in `verify.cpp` is the
  preserved signature-store mirror equality surface, not inline-asm ordinary
  binding agreement.
- The remaining `mirror.str()` checks in `exact_plain_scalar_mirror(...)` are
  integer mirror/output consistency checks, not floating scalar
  classification.
- `function_signature_line(...)` parsing of `signature_text`,
  `aggregate_signature_param_mirror_matches_type(...)` byval fragment checks,
  and direct aggregate signature mirror checks are compatibility/output
  validation surfaces; leave them alone unless separately selected.
- Step 3 accepted commits: `199974299`, `7ade00edd`, `613c40c28`,
  `c7274edf3`, `f05f8d806`, and `7338180ee`.

## Proof

Accepted Step 3 packet proof for each slice:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`.

Accepted Step 3 checkpoint proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|verify_tests_)'`
passed 12/12.
