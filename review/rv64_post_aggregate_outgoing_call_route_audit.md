# RV64 Post-Aggregate Outgoing-Call Route Audit

- Review scope: `src/backend/riscv/codegen/calls.cpp`, `src/backend/riscv/codegen/riscv_codegen.hpp`, `tests/backend/backend_shared_util_tests.cpp`, `src/backend/riscv/codegen/variadic.cpp`, `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`, `plan.md`, `todo.md`
- Chosen review base: `79d6b995` (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to aggregate-struct outgoing-call family`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `79d6b995` as the last canonical lifecycle checkpoint, and `git log --oneline --grep='[plan_change]|[todo_change]|[idea_open_change]' -- plan.md todo.md ideas/open/` shows it is the route-selection commit that activated the current aggregate/struct outgoing-call family. `plan.md` and `todo.md` are unchanged after that point, so there is no newer lifecycle checkpoint for this route.
- Commits reviewed since base: `3`
  - `4f2f53ac rv64: wire aggregate stack outgoing-call staging slice`
  - `d43fd5af rv64: wire aggregate register outgoing-call staging slice`
  - `ee250381 rv64: wire float aggregate outgoing-call staging slice`

## Findings

### No blocking findings in reviewed scope

1. The current diff still matches the active route selected in `plan.md` / `todo.md`: stack-side aggregate staging, GP register-side aggregate staging, and LP64D float/mixed aggregate register staging in `calls.cpp`, with the minimum shared header/test support and no spill into parked broader families. Evidence:
   - stack aggregate staging added in [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:128) through [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:227)
   - aggregate register and float/mixed register staging added in [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:230) through [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:438)
   - shared float-aggregate metadata widened only as needed in [src/backend/riscv/codegen/riscv_codegen.hpp](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:123)
   - focused proof coverage added in [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1528), [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1589), and [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1645)
   - parked `variadic.cpp` remains untouched, consistent with the route guardrails in [todo.md](/workspaces/c4c/todo.md:8)

2. The implementation now mirrors the remaining aggregate/struct outgoing-call cases visible in the translated Rust reference for this family. The newly landed C++ paths cover the same aggregate classes the reference handles in the selected area: `StructByValStack`, `LargeStructStack`, `StructSplitRegStack`, `StructByValReg`, `StructSseReg`, `StructMixedIntSseReg`, and `StructMixedSseIntReg`; compare [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:165), [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:342), and [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:364) against [ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:63), [ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:309), and [ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:362).

3. I found one pre-existing divergence that is not introduced by this route: the Rust reference invalidates the accumulator cache before deferred GP loads in `emit_call_reg_args_impl(...)`, while the current C++ still does not. That code was already absent at the base checkpoint and is not part of the aggregate-family diff, so it is not route drift from these three commits. Evidence: current [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:301) vs reference [ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:233).

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `acceptable`
- Route completeness judgment: the active aggregate/struct outgoing-call family selected by lifecycle commit `79d6b995` is now complete within the bounded route described by the current `plan.md` and `todo.md`.
- Adjacent bounded follow-on: none inside the same outgoing-call aggregate/struct family. The reviewed diff exhausts the aggregate classes explicitly named by the route, and any next work would move into already parked broader families such as `variadic.cpp`, broader call-result expansion, broader memory, or broader comparison work rather than another truthful packet on this exact route.

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: execution completed the currently selected broader outgoing-call aggregate/struct family, so continuing without a lifecycle rewrite would leave the branch past its truthful route checkpoint.
