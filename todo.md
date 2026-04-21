# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Scalar Prepared/Emitter Seam
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Step 2.1 tested the narrowed cross-block `SExt I8->I32` theory directly and
corrected the current ownership boundary again. A generic cast-side fallback
for authoritative prepared `i8` homes compiled and preserved the existing
`backend_x86_handoff_boundary` guard asm, but the clean-tree proof for
`c_testsuite_x86_backend_src_00204_c` did not change at all. That means the
cast seam is real but still downstream of the canonical route: `match` only
reaches that cast after temporarily relaxing the param-bearing single-join
countdown reservation. The attempted cast-only repair was reverted, so this
packet ends with a cleaner blocker note and no code changes.

## Suggested Next

Build the next idea-60 packet around the canonical param-bearing single-join
countdown reservation in `prepared_module_emit.cpp`, with the cross-block
`SExt I8->I32` cast seam kept in scope as the immediately downstream follow-on.
The new evidence is concrete: a cast-only repair is not on the active clean
route yet, so the next accepted slice must first make the param-bearing
`match` route reach the generic local-slot renderer under the normal proof
path. Keep the nearest backend handoff coverage on the same subset and be
ready to continue into `render_prepared_cast_inst_if_supported` only if the
reservation repair exposes that cast seam on the clean tree.

## Watchouts

- Do not ship the reverted cast-only fallback. It compiled and kept
  `backend_x86_handoff_boundary` green, but it did not move `00204` on the
  clean route because the canonical path still stops earlier.
- Do not forget the downstream cast seam. Once the param-bearing countdown
  reservation is relaxed generically, `logic.end.8` still falls out on a
  cross-block `SExt I8->I32` that cannot rely only on the same-block
  `current_i8_name` carrier.
- Keep the blocker in idea 60. The route remains inside x86 structural
  dispatch after the authoritative prepared handoff, and none of the temporary
  probes pushed ownership back into idea 61 or another upstream leaf.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
'^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
`backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00204_c` still failed with the same idea-60
scalar restriction on the reverted clean tree. Supporting investigation tried
a generic cross-block `SExt I8->I32` cast fallback in the x86 local-slot
renderer; it compiled and preserved `backend_x86_handoff_boundary`, but it did
not change the canonical `00204` proof result. That confirms the cast seam is
still downstream of the clean-route param-bearing countdown reservation, even
though temporary probes continue to show the cast as the next blocker once that
reservation is relaxed. Canonical proof log: `test_after.log`.
