Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Classify Step 4 no-breadth residuals

# Current Packet

## Just Finished

Step 4 is complete with a no-breadth blocker. The saved cast rerun produced
one passing cast row (`src/p18298.c`). The narrowed width-preserving follow-up
set is 18 remaining `rv64-consumer:width-preserving-zext-i32-to-i32` rows, 3
remaining `rv64-consumer:width-preserving-trunc-i32-to-i32` rows, and separate
`src/pr81556.c` `RV64_BACKEND_RUNTIME_MISMATCH` evidence. Other cast residuals
remain in ptrtoint and floating-policy buckets. The 60 nearby non-cast guard
rows remained failed in non-cast owner classes.

Reviewer report `review/idea623_cast_breadth_review.md` says the Step 3
same-width i32 GPR move helper is narrow semantic progress and not
testcase-overfit, but the route must be narrowed before more implementation.

## Suggested Next

Execute Step 5 as a classification-only packet. Build a residual table or
artifact covering all 18 zext rows, all 3 trunc rows, and `src/pr81556.c`.
For each residual, record the source file, operation, Step 2 owner bucket,
Step 4 failure class, emitted diagnostic, first still-missing owner fact, and
why it did not follow the passing `src/p18298.c` path.

Do not edit implementation files in this packet.

## Watchouts

- Do not add or widen RV64 cast consumer lowering before the Step 5
  classification is complete.
- Treat `src/pr81556.c` separately from the still-unsupported `CastInst` rows:
  it has moved to runtime mismatch evidence.
- Keep the 60 non-cast guard rows as boundary evidence only.
- Do not use expectation, unsupported-marker, allowlist, timeout, accounting,
  or named-case-only changes as progress.

## Proof

This lifecycle rewrite is plan/todo-only and requires no build proof.

Next executor proof should be classification evidence, preferably recorded in
`todo.md` with any generated artifact paths. If the supervisor delegates a
command, write the result to `test_after.log` unless another artifact is
explicitly chosen.
