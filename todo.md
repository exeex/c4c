Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by rerunning the full `x86_backend` checkpoint after
landing the bounded local-memory `gep` lane. The label now reports `37/220`
passing and `183/220` failing, which is a truthful improvement over the prior
`32/220` checkpoint. The completed `gep` cluster still holds inside that
broader run:
`c_testsuite_x86_backend_src_00032_c`,
`c_testsuite_x86_backend_src_00073_c`, and
`c_testsuite_x86_backend_src_00130_c` all remain passing on the x86 backend
path.

## Suggested Next

Start the next bounded packet on the adjacent local-memory store family rather
than widening to unrelated prepared-module limits: inspect
`c_testsuite_x86_backend_src_00042_c` and
`c_testsuite_x86_backend_src_00046_c` as the next same-neighborhood pair and
verify whether one honest shared store-lane repair can move them together
without reopening the completed `gep` lane.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep the first slice in the local-memory family with already-supported
  control flow.
- Shared `lir_to_bir` capability growth should explain the result more than any
  x86-local patching.
- The checkpoint improved truthfully by five passes, but the dominant remaining
  fail surface is still split across capability families, not isolated tests.
- `00037` is adjacent in numbering but still fails in the `scalar-cast
  semantic family`; do not silently fold it into the next local-memory packet.
- The next nearby local-memory blockers are `00042` and `00046`, both still
  failing in the `store local-memory semantic family`.
- VLA/control-flow-heavy local-memory cases such as `00143` and `00207` still
  fail in the `alloca local-memory semantic family`; they remain a later lane.
- Many other remaining failures are still prepared-module x86 limits such as
  direct-immediate return, minimal single-block return, or single-function
  module restrictions. Do not disguise those as local-memory progress.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -L x86_backend --output-on-failure >> test_after.log 2>&1`.
The build completed, then the `x86_backend` checkpoint finished with `37/220`
passing and `183/220` failing. The repaired `gep` cluster remained green
inside that broader run, and the adjacent blocker families called out above
were confirmed from the same log.
Proof log:
`test_after.log`.
