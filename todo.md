# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Prepared-Module Or Call-Bundle Seam
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Plan Step 2.1 re-inspection from the improved tree isolated the next earliest
still-owned idea-61 seam after the scalar float and focused scalar `F128`
helper lanes. `00204.c` reaches the long-double aggregate helper family in
`arg()` before any return-value or `va_arg` coverage, but the bounded
same-module helper prefix still only has focused scalar float and scalar
`F128` local-materialization/direct-extern support. The current synthetic
coverage proves one `F128` parameter copied through a helper local slot into a
direct-extern call, but it does not cover by-value same-module long-double
aggregates such as `struct hfa32`, `struct hfa33`, or `struct hfa34`, and the
helper renderer has no aggregate/byval long-double ingress path analogous to
the new scalar `F128` lane.

## Suggested Next

Take the next Step 2.1 packet on bounded same-module helper consumption for
by-value long-double aggregates, starting with focused boundary coverage for a
same-module helper that stores and forwards `struct hfa32` / `struct hfa34`
through helper locals before calling `printf`. Prove that aggregate long-double
helper params, not scalar `F128` ingress, are now the earliest still-owned
rejector before widening into return-bundle or `myprintf` / `va_arg` work.

## Watchouts

- The current tree now has three distinct pieces in place for the focused
  scalar long-double lane: 16-byte helper-local admission, authoritative stack
  destination payload for stack-passed call args, and consumer-side scalar
  `F128` param ingress from stack-backed `param.abi`. Reopening any one of
  those as the “missing seam” would be route churn.
- The relevant upstream evidence is in
  `src/backend/bir/lir_to_bir_calling.cpp`,
  `src/backend/prealloc/prealloc.hpp`,
  `src/backend/prealloc/regalloc.cpp`,
  `src/backend/prealloc/legalize.cpp`, and
  `src/backend/prealloc/target_register_profile.cpp`: they now align the
  prepared arg-side scalar `F128` route with stack-backed x86 ownership
  strongly enough for the focused helper boundary coverage to pass.
- The new route evidence comes from `00204.c` itself: `arg()` calls
  `fa_hfa31`..`fa_hfa34`, `fa3`, and `fa4` before the later return-value and
  `myprintf` / `va_arg` sections. That ordering makes long-double aggregate
  helper params the earliest remaining owned family to prove next.
- Do not claim progress by adding another single-`printf("%.1Lf")`-shaped
  helper matcher or by hard-coding x87 stack setup only inside the prepared
  helper renderer. The next seam is aggregate/byval long-double helper
  consumption, not another scalar `F128` spelling variant.
- Do not paper over the remaining rejector by skipping helper emission or
  adding another named-case matcher; the helper-prefix contract is still
  all-or-nothing for non-entry defined functions in the bounded multi-defined
  lane.

## Proof

Ran the delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'; } > test_after.log 2>&1`.
Current proof matches the final workspace state: `backend_x86_handoff_boundary`
passes with both the stronger float helper fixture and the new focused `f128`
helper fixture, while `c_testsuite_x86_backend_src_00204_c` still fails with
the same `minimal i32 return function` diagnostic. This inspection packet
therefore did not add code, but it narrowed the next still-owned route to the
by-value long-double aggregate helper family; `test_after.log` remains the
proof log path.
