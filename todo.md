Status: Active
Source Idea Path: ideas/open/645_rv64_branch_residual_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Residual Terminator Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` refreshed the eight representative branch residual rows.
The object route still fails all eight at the same first consumer boundary:
`unsupported_terminator_fragment: BIR terminator requires unsupported RV64
object lowering`.

Evidence was captured under
`build/agent_state/645_step1_terminator_refresh/`, with one directory per row
containing `object_case.log`, `direct_obj.log`, `dump_bir.txt`, and
`dump_prepared_bir.txt`. A compact rollup is in
`build/agent_state/645_step1_terminator_refresh/summary.md`.

Grouped residual shapes:

- Shared in-scope family: prepared `cond_branch` with `branch_condition
  kind=fused_compare can_fuse_with_branch=yes`. All eight rows contain at
  least one fused-compare branch.
- Stack-backed pointer/condition fused compares with available prepared
  branch-stack-load authority appear in `src/loop-2e.c`, `src/pr39100.c`,
  `src/20000314-3.c`, `src/20140828-1.c`, `src/20050125-1.c`,
  `src/930930-1.c`, and `src/20060910-1.c`.
- Rows with select/join carrier publication boundaries also appear:
  `src/pr39100.c`, `src/20050125-1.c`, `src/20080519-1.c`,
  `src/930930-1.c`, and `src/20060910-1.c`. These should not be treated as
  branch lowering proof until Step 2 decides whether a terminator family can
  advance independently.
- Freshness/clobber-policy gaps remain visible in some secondary branch
  operands, including `missing_policy` in `src/pr39100.c`,
  `src/20060910-1.c`, and `src/20080519-1.c`, and
  `missing_stack_clobber_safety` in `src/930930-1.c`. Do not paper over those
  gaps inside RV64 lowering.

Step 1 found at least one shared terminator family in scope for semantic RV64
lowering comparison: prepared fused-compare conditional branches whose needed
stack operand authority is already `available`.

## Suggested Next

Execute Step 2: compare the prepared fused-compare conditional branch shapes
against existing RV64 terminator-fragment support and closed idea 611. Select
the smallest same-family branch shape that can lower using only explicit
`PreparedBranchCondition` plus available `branch_stack_load_authority`, or
reclassify the select/join and missing-authority rows to their more precise
owners.

## Watchouts

- Do not publish new branch freshness or clobber-safety authority in this
  idea.
- Do not accept stack branch operands from stack offsets, frame homes, final
  assembly shape, or assumed no-clobber behavior.
- Do not add named-case handling for the representative source files.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or pass/fail accounting as capability progress.
- `emit_riscv_prepared_fused_compare_branch()` currently falls back through
  generic move emission; Step 2 should verify whether the missing object-route
  fragment is a prepared branch-stack-load operand consumption gap, a label or
  target issue, or a select/join carrier owner.
- Keep the first implementation target shape-based. Good probe candidates are
  rows with available stack-load authority and no required select carrier at
  the first branch, such as `src/20140828-1.c`, `src/20000314-3.c`, and
  `src/loop-2e.c`.

## Proof

Classification probe command:

```bash
cmake --build --preset default
ALLOWLIST=build/agent_state/645_step1_terminator_refresh/allowlist.txt \
  BUILD_DIR=build VERBOSE_FAILURES=1 \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Additional evidence dumps per row:

```bash
./build/c4cll --dump-bir --target riscv64-linux-gnu <row>
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu <row>
./build/c4cll --codegen obj --target riscv64-linux-gnu <row> -o <probe>.o
```

`test_after.log` contains the eight-row proof/probe artifact. Result:
build passed, BIR and prepared-BIR dumps returned rc 0 for all eight rows,
and the object-route check returned nonzero as expected with 0/8 passing
because each row still fails at `unsupported_terminator_fragment`.
