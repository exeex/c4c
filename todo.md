Status: Active
Source Idea Path: ideas/open/452_select_edge_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 452. Supporting artifact:
`build/agent_state/452_step4_residual_disposition/disposition.md`.

Fresh `20010329-1` probing still fails at
`unsupported_move_bundle_target_shape`, matching the Step 2/3 boundary.
Register/immediate select-edge compare source rematerialization is already
covered by existing object emission and focused tests. The remaining `%t18 ->
%t22` row depends on `%t17 = inttoptr i32 %t16 to ptr`, and `%t17` has a
stack-slot pointer home. That makes the first owner stack-slot pointer
dependency materialization, not more register/immediate rematerialization.

## Suggested Next

Plan-owner should close idea 452 as complete for register/immediate
select-edge source-producer rematerialization. Route the remaining
`%t18/%t17` representative failure to stack-slot pointer dependency
materialization, either under `ideas/open/451_stack_home_branch_operand_materialization.md`
if that idea owns the overlap or under a narrower follow-up.

## Watchouts

- Do not copy `%t18` on the predecessor edge unless edge availability is
  proven.
- Do not infer source-producer dependencies from raw BIR shape, block order,
  filenames, function names, or one prepared dump layout.
- Keep stack-home branch operand materialization routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md` unless Step 2
  explicitly accepts a narrow overlap as part of edge rematerialization.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Keep `20000622-1` out of the first packet while its first owner is
  instruction-side lowering.
- Do not treat a successor-defined compare result's register home as
  predecessor-edge availability.
- Do not let the later `%t32/%t46` register-compatible candidates hide the
  first `%t18/%t17` stack-slot dependency decision.
- Do not admit stack-slot pointer dependencies in the first implementation
  packet; `%t18 -> %t22` remains the fail-closed boundary case.
- Step 3 must require prepared source-producer identity and cannot discover
  compare/cast dependencies from raw BIR alone.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 4 residual-disposition validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
