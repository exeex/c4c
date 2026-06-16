Status: Active
Source Idea Path: ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Route structured lowering through metadata

# Current Packet

## Just Finished

Step 2 from `plan.md` completed: routed structured call-argument layout and
scalar pre-classification through structured type metadata when present, leaving
rendered `alignstack(...)` suffix parsing only in explicit legacy/no-ref
fallback branches.

Concrete changes:

- `src/backend/bir/lir_to_bir/calling.cpp`: added local helpers to read
  structured call-argument type text from `LirCallArg::type_ref` or
  `arg_type_refs`, to select aggregate layouts through structured type refs,
  and to use rendered suffix stripping only when no structured type metadata is
  available.
- `src/backend/bir/lir_to_bir/calling.cpp`: changed metadata-bearing
  `lower_byval_call_arg_layout` branches so they no longer compare stripped
  rendered carrier text. `StructNameId` refs resolve through structured layout
  lookup; no-ID explicit `ptr byval(...)` refs still fail closed; no-ID
  non-byval aggregate carriers such as `[2 x float]` lower from clean type-ref
  text.
- `src/backend/bir/lir_to_bir/calling.cpp`: changed both scalar
  pre-classification sites to classify from structured type text when present,
  with rendered suffix stripping retained only as the no-metadata fallback.
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`: cleaned the AArch64
  carrier-array fixtures so `arg_type_refs` do not carry rendered
  `alignstack(...)` suffixes, made `args_str` deliberately stale with
  `alignstack(16)`, and asserted the lowered ABI uses structured metadata
  alignment `8`.

## Suggested Next

Execute Step 3 from `plan.md`: quarantine or remove the remaining
`strip_call_arg_abi_type_suffix` uses. The remaining production uses are the
helper definition, `scalar_call_arg_type_text` no-metadata fallback, and the two
explicit `lower_byval_call_arg_layout` legacy/no-ref layout branches.

## Watchouts

`LirCallArg::aarch64_stack_align_bytes` remains the only structured
`alignstack` authority; `LirTypeRef`/`arg_type_refs` are type identity only.
No-ID explicit `ptr byval(...)` type refs must continue to fail closed, as
proved by the existing metadata-rich byval fail-closed notes test. Raw/no-ref
fallbacks still exist and should be fenced or removed in Step 3 without
broadening compatibility.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'`

Result: build completed and `backend_lir_to_bir_notes` passed. Full output is
preserved in `test_after.log`.

Supervisor broader guard:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: non-regressive before/after comparison with
`check_monotonic_regression.py --allow-non-decreasing-passed`; both runs had
159 passed, 21 failed, 180 total, with no new failures. The 21 failures are
pre-existing backend CLI/route semantic BIR admission failures and remain a
closure-time acceptance blocker for the broader 286/288 subset.
