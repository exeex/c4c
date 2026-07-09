Status: Active
Source Idea Path: ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Compare Against Existing Terminator Support

# Current Packet

## Just Finished

Step 2 of `plan.md` compared the refreshed fused-compare residuals against
existing RV64 object terminator support and closed idea 611.

Evidence was captured under
`build/agent_state/645_step2_terminator_compare/`. The comparison summary is
`build/agent_state/645_step2_terminator_compare/summary.md`, and selected
prepared-dump excerpts are in
`build/agent_state/645_step2_terminator_compare/selected_candidate_excerpts.txt`.

Existing support already handles:

- prepared `CondBranch` labels and local relocations through
  `fragment_for_prepared_terminator()`;
- fused integer and fused floating compare branches;
- fused pointer register/null compare branches;
- stack-homed pointer compare operands when explicit branch-stack-load
  freshness is selected.

The first missing piece is not label/target lowering. It is the prepared fused
pointer branch consumer path accepting a fused condition value and compared
pointer operand whose stack loads are explicitly authorized by
`branch_stack_load_authority`, without inferring from stack offsets, frame
homes, or final assembly shape.

Selected Step 3 target shape:

- representative proof row: `src/20140828-1.c`;
- function `main`, block `entry`;
- `branch_condition entry kind=fused_compare condition=%t7 compare=ne ptr %t4, %t6`;
- `%t4` is register-backed;
- `%t6` has available `branch_stack_load_authority role=rhs`;
- `%t7` has available `branch_stack_load_authority role=condition`;
- no select/join carrier is needed at the first branch.

This is a same-family extension of idea 611's terminator-consumer route:
prepared fused pointer compare branch lowering with explicit prepared authority
for the stack-backed condition value and exactly one stack-backed pointer
operand. It must remain shape-based, not `src/20140828-1.c` specific.

## Suggested Next

Execute Step 3: lower the selected prepared fused pointer compare branch shape
in the RV64 object route using only explicit `PreparedBranchCondition` plus
available `branch_stack_load_authority` for the condition value and the
stack-backed LHS/RHS operand. Add focused positive coverage for the shape and
negative coverage that rejects missing or ambiguous condition/operand
branch-stack-load authority.

## Watchouts

- Do not publish new branch freshness or clobber-safety authority in this
  idea.
- Do not accept stack branch operands from stack offsets, frame homes, final
  assembly shape, or assumed no-clobber behavior.
- Do not add named-case handling for the representative source files.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or pass/fail accounting as capability progress.
- `plan_prepared_fused_pointer_branch_publication()` currently rejects
  non-GPR condition homes as `UnsupportedConditionHome`; for fused compare
  branches, Step 3 should consume explicit `role=condition` authority rather
  than requiring the condition value to be preloaded in a GPR.
- `src/20050125-1.c`, `src/pr39100.c`, `src/20080519-1.c`,
  `src/930930-1.c`, and `src/20060910-1.c` expose select/join carrier,
  `missing_policy`, or `missing_stack_clobber_safety` boundaries on secondary
  branches. Do not use those as Step 3 positive proof or paper over those
  owners inside RV64 lowering.
- `src/20000314-3.c` and `src/loop-2e.c` are useful same-family secondary
  probes after the selected `src/20140828-1.c` shape is implemented.

## Proof

Step 2 comparison proof command:

```bash
rm -f test_after.log && (cmake --build --preset default && \
ALLOWLIST=build/agent_state/645_step1_terminator_refresh/allowlist.txt \
  BUILD_DIR=build VERBOSE_FAILURES=1 \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1
```

`test_after.log` contains the eight-row object-route proof/probe artifact.
Result: build passed; the object-route check returned nonzero as expected with
0/8 passing because all rows still fail at `unsupported_terminator_fragment`.
This is acceptable for Step 2 classification because no implementation change
was made.
