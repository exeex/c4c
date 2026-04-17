Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by running the `x86_backend` checkpoint after the
bounded local-slot guard-chain lane landed. The backend label now reports
`32/220` passing and `188/220` failing, which is a truthful improvement over
the `18/220` pass snapshot recorded in the source idea. The original proving
cluster still holds inside that broader checkpoint:
`c_testsuite_x86_backend_src_00005_c`,
`c_testsuite_x86_backend_src_00020_c`, and
`c_testsuite_x86_backend_src_00103_c` remain passing on the x86 backend path.

## Suggested Next

Start the next bounded packet on the adjacent local-memory `gep` family rather
than widening x86 control flow: inspect `c_testsuite_x86_backend_src_00032_c`,
`c_testsuite_x86_backend_src_00073_c`, and
`c_testsuite_x86_backend_src_00130_c` as a same-family cluster and verify
whether one honest constant-offset/element-address lowering repair can move
them together through shared `lir_to_bir` semantics.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep the first slice in the local-memory family with already-supported
  control flow.
- Shared `lir_to_bir` capability growth should explain the result more than any
  x86-local patching.
- The new checkpoint shows the next nearby same-family blockers are still
  semantic local-memory gaps, not x86 renderer regressions: `00032`, `00073`,
  and `00130` fail in the `gep local-memory semantic family`.
- `00037` is adjacent in numbering but fails in the `scalar-cast semantic
  family`; do not silently fold it into the next `gep` packet.
- Aggregate or union-backed local-store cases such as `00042` and `00046`
  still fail in the `store local-memory semantic family`, so keep them out of
  a `gep`-focused packet unless one shared lowering root cause clearly covers
  both.
- VLA/control-flow-heavy local-memory cases such as `00143` and `00207` still
  fail in the `alloca local-memory semantic family`; they remain a separate
  later lane.
- Many remaining failures are still x86 prepared-handoff limits such as
  single-function module or minimal-control-flow restrictions. Do not disguise
  those as semantic local-memory progress.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -L x86_backend --output-on-failure >> test_after.log 2>&1`.
The build completed, then the `x86_backend` checkpoint finished with
`32/220` passing and `188/220` failing. The proving cluster remained green
inside that broader run, and the nearby blockers called out above were
confirmed from the same log. Proof log:
`test_after.log`.
