Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Stack-Carried Pointer Evidence

# Current Packet

## Just Finished

Activated plan Step 1, `Refresh Stack-Carried Pointer Evidence`.

## Suggested Next

Executor packet: refresh object, prepared-BIR, disassembly, and runtime
evidence for `%t6` in `src/20140828-1.c` and `%t23` in `src/loop-2e.c`;
record each selected stack slot, source value, consumer branch, current
publication or materialization facts, and the precise first owner.

## Watchouts

- Do not reopen RV64 terminator-fragment admission from idea 645.
- Do not infer pointer freshness or materialization from stack offsets, final
  assembly shape, source spelling, local names, diagnostics, testcase identity,
  runtime outcomes, or pass/fail accounting.
- If refreshed evidence belongs to the direct-global stack-backed pointer
  branch boundary, request lifecycle switch to idea 654 instead of broadening
  this plan.

## Proof

Lifecycle activation only; no code validation was run.
