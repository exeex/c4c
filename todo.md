# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 5 Validate The Route packet in `todo.md` by replacing the
single-test `^backend_x86_handoff_boundary$` acceptance note with a broader
`^backend_` checkpoint, preserving `test_after.log`, and recording that the
broader backend bucket currently reproduces the same four known route failures
while `backend_x86_handoff_boundary` still passes inside that broader scope.

## Suggested Next

Do not queue another adjacent Step 3 compare-join passthrough-family packet
yet. The next accepted packet should either remove one of the four current
`^backend_` failures so the broader checkpoint can become monotonic, or move
to a materially different prepared control-flow consumer seam only after the
supervisor explicitly accepts the current broader-proof state.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- A route review judged the route `drifting`, not because Step 3 failed, but
  because packet selection and proof discipline narrowed too far around one
  compare-join passthrough family. Do not queue another adjacent
  passthrough-coverage packet until the broader proof gap is addressed.
- Do not solve remaining compare-join gaps with x86-side CFG scans,
  testcase-shaped matcher growth, or broad multi-block rediscovery. This
  family should only allow one extra empty passthrough after an already-
  authoritative compare lane when the prepared branch labels and join-transfer
  ownership already identify the real source edges, including the
  fixed-offset pointer-backed selected-value chain return-context variant that
  now has both true-lane and false-lane passthrough proof for the plain helper
  path and the paired EdgeStoreSlot carrier without any backend change.
- Keep follow-on work focused on places where prepared branch labels and join
  ownership are already authoritative; do not reintroduce source-label
  equality checks, local join bundle reconstruction, or emitter-local semantic
  recovery.
- Treat the optional-empty-passthrough recognition in shared helpers as route
  debt, not as a license to keep cloning more same-shape variants. Follow-on
  work should either prove the broader route or strengthen prepared ownership
  more generally.
- The broader `^backend_` checkpoint is stable but not acceptance-green: the
  bucket still fails in
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
  Those failures sit outside this packet's owned files.
- `test_before.log` and `test_after.log` are byte-identical for `^backend_`,
  so the broader bucket did not regress, but the monotonic regression checker
  still returns failure because pass count stayed flat instead of increasing.
- When a test helper appends passthrough blocks to `function.blocks`, reacquire
  any cached block pointers before calling prepared-helper classifiers; this
  packet needed that harness-only fix to keep the proof scoped to the intended
  ownership contract.
- The review's broader-proof requirement is now satisfied in scope, but not in
  acceptance quality: future Step 3 same-family work should not treat this as
  a green milestone until the supervisor decides how to handle the stable
  broader-bucket failures.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_' | tee test_after.log`.
This broader backend checkpoint preserves `test_after.log` for the full
`^backend_` bucket and reproduces the same four failing route cases already
present in `test_before.log`; `backend_x86_handoff_boundary` still passes
within that broader subset. A follow-on monotonic comparison with
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
--before test_before.log --after test_after.log` reported zero new failing
tests and zero resolved failing tests, but still returned failure because the
pass count did not strictly increase.
