Status: Active
Source Idea Path: ideas/open/505_select_publication_stack_home_intent_support.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Stack-Home Intent Construction

# Current Packet

## Just Finished

Step 1 inspected `EdgePublicationMoveIntent` construction for the two
select-publication stack-home rows from idea 504 without implementation
changes.

`src/builtin-constant.c` has a concrete pointer stack source
(`%t8`, slot `#1`, offset `8`, type `ptr`, size `8`) and a GPR destination
(`%t10`, `t0`) on edge `tern.then.end.4 -> tern.end.7`, but the adapter only
has same-width `i32` and concrete `i64` stack-source-to-register policies. It
clears the concrete stack-source fields and returns
`intent_status_unsupported_source_home`.

`src/pr58726.c` has a direct GPR source (`%p.p`, `a0`) and a concrete i16 stack
destination (`%t11`, slot `#4`, offset `4`, size `2`) on edge
`tern.then.end.5 -> tern.end.8`, but the adapter's destination stack-slot
branch only accepts 4-byte destinations. It returns
`intent_status_unsupported_destination_home` before populating destination
stack fields.

Durable evidence was written under
`build/agent_state/505_step1_stack_home_intent_construction/`.

## Suggested Next

Execute Step 2 by adding narrow select-publication stack-home intent support
inside `EdgePublicationMoveIntent` construction: pointer/XLEN concrete
stack-source to GPR destination for `src/builtin-constant.c`'s shape, and
direct GPR source to 1/2/4-byte concrete stack destination for
`src/pr58726.c`'s shape. Keep this to intent construction; do not implement a
separate final RV64 select-publication materialization consumer.

## Watchouts

- Do not implement RV64 select-publication materialization under idea 505.
- Do not fold generic immediate materialization from `src/pr37924.c` into this
  select-publication intent work.
- Do not infer homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Preserve fail-closed behavior for missing edge/destination identity, missing
  concrete stack offset/size, non-GPR destination placement, stack-to-stack
  select-publication without an explicit scratch policy, and large offsets
  without an explicit address materialization policy.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 1 proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
