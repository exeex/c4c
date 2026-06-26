Status: Active
Source Idea Path: ideas/open/369_rv64_object_route_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit First Terminator Fragment

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: audited the first prepared terminator fragment
behind the current RV64 object-route `unsupported_terminator_fragment`
failures for `src/20000224-1.c` and `src/20000112-1.c`.

Evidence is recorded in:

- `test_after.log`
- `build/agent_state/369_step1_terminator_audit.summary.log`
- `build/agent_state/369_step1_terminator_audit.prepared_bir.log`
- `build/rv64_gcc_c_torture_backend/src_20000224-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`

## Suggested Next

Implement the first allowlist-order prepared terminator shape: RV64 object
lowering for a fused-compare conditional branch with signed greater-than i32
register operands, observed as `compare=sgt i32 %t0, %t1` on
`src/20000224-1.c` function `test` block `block_1`.

## Watchouts

- `src/20000224-1.c` first reaches `function=test block=block_1
  terminator=cond_branch` with prepared branch condition
  `fused_compare condition=%t2 compare=sgt i32 %t0, %t1`; the local owner is
  RV64 object terminator lowering because `rv64_branch_funct3` currently lacks
  `Sgt`.
- `src/20000112-1.c` first reaches `function=special_format block=entry
  terminator=cond_branch` with prepared branch condition
  `fused_compare condition=%t1 compare=ne ptr %t0, 0`; `Ne` is mapped, but the
  pointer-typed null operand is not accepted by the current immediate/value
  move helpers.
- Lower the shape semantically, for example by operand normalization to a
  supported RV64 branch predicate. Do not key behavior on testcase names,
  labels, or prepared-BIR text matching.

## Proof

Ran the supervisor-selected proof:

`mkdir -p build/agent_state && printf '%s\n' 'src/20000224-1.c' 'src/20000112-1.c' > build/agent_state/369_step1_terminator_audit.allowlist.txt && ( cmake --build build --target c4cll && ALLOWLIST=build/agent_state/369_step1_terminator_audit.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh ) > test_after.log 2>&1`

Result: expected nonzero proof while auditing current unsupported failures;
`test_after.log` contains the fresh run with both cases failing at
`unsupported_terminator_fragment`.
