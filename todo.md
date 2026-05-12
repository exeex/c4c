# Current Packet

Status: Active
Source Idea Path: ideas/open/180_aarch64_direct_lir_aggregate_type_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Require Structured Facts on the Selected Route

## Just Finished

Step 2 repaired structured aggregate metadata enforcement so it remains bounded
to the selected AArch64/direct-LIR function signature ABI route in
`call_abi.cpp`.

Completed work:
- `lower_return_info_from_type(...)` now accepts the optional signature
  `LirTypeRef`; metadata-rich aggregate returns resolve layout through
  `lookup_backend_aggregate_type_ref_layout_result(...)` and `StructNameId`.
- AArch64 function definitions with `signature_return_type_ref` now use
  signature return metadata as the return ABI authority instead of reparsing
  return terminator text.
- AArch64 structured signature byval parameters now require a valid ID-backed
  aggregate layout at the ABI boundary. Stale, missing, opaque, or no-ID
  metadata-rich aggregate params fail in the function-signature family instead
  of falling through as scalar `ptr` or legacy text.
- Non-AArch64/legacy signature paths pass no enforced type ref into the
  aggregate ABI helper, preserving existing no-ID compatibility for x86/backend
  lowering such as `00204.c::fa_s17`.
- Legacy rendered-text parsing remains fenced to the no-signature-metadata
  fallback path.
- Focused tests cover AArch64 metadata-rich aggregate return success, missing
  and mismatched return IDs, incoming byval success, stale/missing/opaque/no-ID
  byval failures, the legacy no-signature-ref compatibility fallback, and the
  non-AArch64 metadata-rich no-ID compatibility fence.

## Suggested Next

Supervisor can review and commit this Step 2 slice. The next coherent packet is
Step 3/4 consolidation only if the supervisor wants additional route-summary
coverage beyond the focused tests added here.

## Watchouts

- The helper in `call_abi.cpp` intentionally normalizes `ptr byval(...)` only
  so the selected signature ABI route can compare the `StructNameId` against
  structured layout facts. It does not add a new rendered aggregate parser
  authority for metadata-rich inputs.
- On the selected AArch64 route, no-ID compatibility means no structured
  signature type refs are present; a present aggregate signature type ref
  without `StructNameId` fails closed. On non-selected routes, present no-ID
  aggregate signature refs keep legacy text-layout compatibility.
- Call-argument, global, and local-memory aggregate routes were not widened.

## Proof

Ran the supervisor-selected proof exactly:

`set -o pipefail; { cmake --build build --target backend_lir_to_bir_notes_test backend_aarch64_signature_metadata_test frontend_lir_function_signature_type_ref_test && ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_aarch64_signature_metadata|frontend_lir_function_signature_type_ref)$' --output-on-failure; } 2>&1 | tee test_after.log`

Result: passed, `100% tests passed, 0 tests failed out of 3`.

Proof log: `test_after.log`.

Also ran the delegated broader backend validation:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee /tmp/c4c_backend_validation_aarch64_sig_step2_fix.log`

Result: passed, `100% tests passed, 0 tests failed out of 109` with 12 disabled
MIR CLI tests not run by CTest.

Broader validation log:
`/tmp/c4c_backend_validation_aarch64_sig_step2_fix.log`.
