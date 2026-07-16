Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Delete semantic string escape hatches

# Current Packet

## Just Finished

Completed Step 3 packet to remove verifier-side floating scalar semantic text
classification for `LirTypeRef` in `src/codegen/lir/verify.cpp`.

`is_native_scalar_floating_type(...)` now uses `LirTypeRef::kind()` and
`builtin_type()` for `float`, `double`, `x86_fp80`, and `fp128` classification.
The direct `double(double)` helper path now uses typed/native double checks
instead of string-constructed or rendered-text comparisons, and
`exact_plain_scalar_mirror(...)` classifies float/double mirrors through
`builtin_type()` rather than `mirror.str()`.

## Suggested Next

Supervisor should review and commit this Step 3 verifier helper slice if
accepted, then continue with the next remaining Step 3 string escape hatch
candidate. Do not widen this packet into Step 4 adapter removal.

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
- Signature-store mirror text equality is intentionally preserved at the
  function signature store boundary; it is output-boundary consistency, not
  semantic type authority.
- `same_signature_store_type_fact(...)` in `src/codegen/lir/verify.cpp` and
  `LirModule::same_function_signature_entry(...)` behavior were left intact.
- The remaining `mirror.str()` checks in `exact_plain_scalar_mirror(...)` are
  integer mirror/output consistency checks, not floating scalar
  classification.
- `function_signature_line(...)` parsing of `signature_text`,
  `aggregate_signature_param_mirror_matches_type(...)` byval fragment checks,
  and direct aggregate signature mirror checks are compatibility/output
  validation surfaces; leave them alone unless separately selected.
- Clean-search checks for direct `type.str()`/`mirror.str()` floating spelling
  classification and local `LirTypeRef("float"/"double"/"x86_fp80"/"fp128")`
  probes in `verify.cpp` returned no matches.

## Proof

Proof run passed:
`{ cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^frontend_lir_call_type_ref$|^frontend_lir_function_signature_type_ref$'; }
> test_after.log 2>&1`.
`test_after.log` contains the focused CTest subset output with 2/2 selected
tests passing.
The supervisor-selected proof was sufficient for this packet.
