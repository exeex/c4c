# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Scalar Prepared/Emitter Seam
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 2.1 inspected the x86 structural-dispatch seam directly and stopped
without a shippable repair. Temporary local probes confirmed that the reduced
`match` route does reach the generic local-slot renderer once the param-bearing
single-join countdown reservation is relaxed, but the next owned blocker is
immediately deeper in that same x86 path: block `logic.end.8` starts with a
plain `SExt I8->I32` cast whose operand is no longer carried by the
same-block `current_i8_name` fast path. A follow-up attempt to add generic
cross-block `i8` home materialization for that cast compiled, but it regressed
the existing byte-addressed local-slot guard lane and was reverted. The packet
therefore ends with a narrower blocker note and no code changes.

## Suggested Next

Build the next idea-60 packet around generic cross-block `i8` carrier
materialization inside `render_prepared_cast_inst_if_supported` and the
adjacent local-slot state helpers. The route evidence is now concrete: after
the countdown reservation is bypassed for param-bearing functions, `match`
falls out on `logic.end.8` because the `SExt I8->I32` cast cannot consume an
authoritative prepared `i8` home across blocks. Keep the repair generic enough
to preserve the existing byte-addressed guard asm exactly, and only revisit the
countdown reservation once that cast seam is covered by the nearest backend
handoff test.

## Watchouts

- Do not keep the old count-only countdown reservation as the whole story. The
  local-slot renderer is reachable for param-bearing `match`; the next failure
  is the cross-block `i8` cast seam inside that renderer, not missing prepared
  pointer publication.
- Do not ship the broad cast fallback experiment as-is. It regressed the
  existing `backend_x86_handoff_boundary` byte-addressed guard lane before
  helping `00204`, so the next repair must preserve that canonical asm.
- Keep the blocker in idea 60. The route is still inside x86 structural
  dispatch after the authoritative prepared handoff, and the temporary probes
  did not show ownership falling back into idea 61 or another upstream leaf.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
'^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
`backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00204_c` still failed with the idea-60 scalar
restriction on the reverted clean tree. Supporting investigation used temporary
local probes during this packet: they showed that once the param-bearing route
reaches the generic local-slot renderer, the first concrete blocker is a
`SExt I8->I32` cast at `logic.end.8` whose operand is not available through the
same-block `current_i8_name` carrier. The temporary code experiments were
reverted because they either left proof unchanged or regressed
`backend_x86_handoff_boundary`. Canonical proof log: `test_after.log`.
