Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by rerunning the truthful `x86_backend`
checkpoint, re-baselining the remaining failures, and naming the next bounded
family as the single-function prepared-module limit with a narrow direct
helper-call proving cluster (`00021.c`, `00190.c`). The refreshed checkpoint
still records `57/220` passed and `163/220` failed, with the dominant visible
remaining blocker now the emitter's single-function prepared-module boundary.

## Suggested Next

Start an implementation packet for `plan.md` Step 3 that widens the prepared
x86 handoff/emitter from one-function modules to a bounded direct helper-call
lane proven by `c_testsuite_x86_backend_src_00021_c` and
`c_testsuite_x86_backend_src_00190_c`, while keeping function-pointer,
stdio-heavy, and multi-block neighbors unsupported.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Do not silently reuse the completed pointer-backed same-module global lane as
  the active route.
- Treat the broader single-function prepared-module bucket as larger than the
  first lane: keep indirect/variadic multi-function routes like `00189.c` out
  of scope for the first implementation packet.
- Keep stdio-heavy prepared-module neighbors such as `00131.c` and `00154.c`
  separate until the bounded direct helper-call lane is admitted honestly.
- Keep bootstrap scalar globals (`00045.c`) and scalar-control-flow blockers
  (`00051.c`) as adjacent but separate families.

## Proof

Ran `cmake --build --preset default && (ctest --test-dir build -L x86_backend
--output-on-failure || true) > test_after.log 2>&1` to refresh the current
checkpoint evidence. `test_after.log` records the unchanged `57/220` passed,
`163/220` failed `x86_backend` result; the command is intentionally
shell-successful because the label still contains open failures.
