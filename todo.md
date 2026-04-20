# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Semantic/BIR Seam
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the remaining scalar/local-memory semantic seam exposed
by `00113` and `00159`. The backend now admits float `fcmp` through the normal
scalar compare lowerer and lets typed null indirect callees stay on the
semantic call lane instead of bouncing out through the scalar/local-memory
umbrella. That repair is upstream and general rather than testcase-shaped: the
nearest notes coverage now proves both a float local-memory compare lane and a
typed null indirect-call lane lower successfully without re-entering the
scalar/local-memory family. Under the delegated proof, both `00113` and
`00159` leave the old idea-58 scalar/local-memory failure family and now
graduate into the downstream prepared-emitter minimal-return restriction
family.

## Suggested Next

Treat the scalar/local-memory seam as repaired for this packet and route the
graduated cases explicitly: `00113` and `00159` now belong with idea 60's
minimal-return prepared-emitter restriction family instead of staying in idea
58. For idea 58 itself, the next packet should isolate another still-owned
semantic seam such as the scalar-binop family behind `00174`, rather than
reopening float-compare or null-callee work that already reaches prepared x86
consumption.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.
- The new admission is intentionally limited to semantic lowering plus BIR
  representation; it does not claim downstream prepared-emitter support for the
  float-return or indirect-call routes these cases now expose.
- `backend_lir_to_bir_notes` remains the nearest protective backend coverage
  for this seam because it now exercises both admitted float local-memory
  compares and typed null indirect calls without depending on one named
  c-testsuite case.
- `00113` and `00159` should stay out of this runbook unless a later audit
  shows they fell back before prepared x86 consumption again.
- Keep the next packet centered on a still-owned semantic family such as
  scalar-binop, bootstrap-global lowering, or another non-addressing seam;
  route durable gep/addressing ownership to idea 62 when that becomes the more
  precise description.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|c_testsuite_x86_backend_src_00113_c|c_testsuite_x86_backend_src_00159_c)$' | tee test_after.log`.
Current result: `backend_lir_to_bir_notes` passes with the new admitted float
compare and typed null indirect-call lanes covered. `00113` and `00159` no
longer fail with the old idea-58 scalar/local-memory diagnostic; both now fail
downstream with idea-60's minimal-return prepared-emitter restriction. The
delegated proof remains nonzero overall because both cases now stop in a
downstream prepared-emitter family, not because idea 58 still owns them. For
acceptance, that route change is valid progress because this packet removes the
remaining scalar/local-memory seam from idea 58. A broader supervisor-side
`^backend_` check also passed after the patch. This slice is commit-ready for
idea-58 runbook progress even though the delegated proof remains red overall.
