# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` by moving the
materialized compare-join entry-side prepared branch gate behind
`find_prepared_eq_i32_param_zero_branch_condition()`. The compare-join path no
longer open-codes the prepared `param == 0` operand check inline, and the same
prepared-branch helper now also serves the adjacent minimal compare-branch
route.

## Suggested Next

The next small Step 3 packet is to move the compare-join path's
true/false-transfer fetch plus paired return rendering behind one prepared
join-consumer helper so `render_materialized_compare_join_if_supported()`
narrows further without merging with short-circuit topology rules or turning
into Step 4 file cleanup.

## Watchouts

- Do not activate umbrella idea 57 or idea 59 while cleaning this helper
  surface.
- Do not widen this Step 3 packet into generic instruction selection.
- Do not pull in idea 60 value-location work or idea 61 frame/addressing work.
- Do not touch countdown-loop routes in this packet.
- Do not pre-spend Step 4 by turning this into `prepared_module_emit.cpp`
  file-organization cleanup.
- Do not solve coverage gaps with x86 testcase-shaped matcher growth or new
  emitter-local CFG scans.
- `find_materialized_compare_join_context()` is intentionally scoped to the
  joined compare-return route: it validates the authoritative prepared join
  carrier and trailing binary around the prepared join result, but it should
  not absorb the short-circuit route's continuation/topology rules.
- `find_prepared_eq_i32_param_zero_branch_condition()` is scoped to prepared
  `Eq` `i32` conditions over the entry param and zero. If another route needs a
  broader predicate shape, strengthen the helper around prepared branch
  semantics instead of restoring inline operand scans.
- If another consumer path needs extra branch or join facts, strengthen the
  shared prepared-control-flow contract in `prealloc.hpp` rather than growing
  emitter-local scans or CFG-shape recovery.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command for supervisor review.
- Keep the compare-join helper surfaces keyed by prepared branch/join metadata,
  not by local instruction names or join-block scans.
- Treat any future fix here as capability repair, not expectation weakening:
  the joined-branch and loop-countdown routes are covered by
  `backend_x86_handoff_boundary`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
This prepared-branch helper packet passed with the focused handoff-boundary
proof command, and `test_after.log` remains the fresh canonical proof log for
supervisor review.
