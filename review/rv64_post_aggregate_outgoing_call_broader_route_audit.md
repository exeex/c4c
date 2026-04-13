# RV64 Post-Aggregate Outgoing-Call Broader Route Audit

- Review scope: `plan.md`, `todo.md`, `review/rv64_post_aggregate_outgoing_call_route_audit.md`, `src/backend/riscv/codegen/calls.cpp`, `src/backend/riscv/codegen/variadic.cpp`, `src/backend/riscv/codegen/memory.cpp`, `src/backend/riscv/codegen/comparison.cpp`, `src/backend/riscv/codegen/riscv_codegen.hpp`, `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`, `ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs`
- Chosen review base: `77482587` (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending post-aggregate route`)
- Why this base is correct: `scripts/plan_change_gap.sh` reports `77482587` as the last canonical lifecycle checkpoint, and `git log --oneline -- plan.md todo.md ideas/open` shows it is the commit that converted the just-finished aggregate / struct outgoing-call family into the current blocked state. This audit is explicitly about choosing the next route from that blocked checkpoint, so the blocked lifecycle commit itself is the right review base.
- Commits reviewed since base: `0`

## Findings

### 1. No implementation drift exists after the blocked lifecycle checkpoint, so the branch is still truthfully parked and any further execution inside the aggregate-call family would be route drift.

- `git diff 77482587..HEAD` is empty and `git rev-list --count 77482587..HEAD` is `0`.
- The active lifecycle files still describe a blocked post-aggregate checkpoint rather than an executor packet: [plan.md](/workspaces/c4c/plan.md:62), [plan.md](/workspaces/c4c/plan.md:68), [todo.md](/workspaces/c4c/todo.md:9), [todo.md](/workspaces/c4c/todo.md:30).
- The previous route audit already marked the aggregate / struct outgoing-call family complete with no adjacent bounded follow-on on that exact route: [review/rv64_post_aggregate_outgoing_call_route_audit.md](/workspaces/c4c/review/rv64_post_aggregate_outgoing_call_route_audit.md:20).

### 2. The narrowest honest continuation from this checkpoint is the broader translated calls result-store expansion centered on the parked `I128` / `F128` paths in `emit_call_store_result_impl(...)`.

- The remaining visible gap on the active translated calls surface is explicit: `emit_call_store_result_impl(...)` still returns immediately for `IrType::F128` and `IrType::I128` in [src/backend/riscv/codegen/calls.cpp](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:471).
- The translated Rust reference shows that this exact owner surface already has a concrete broader continuation for those cases: paired `a0`/`a1` stores for `I128`, and paired store plus `__trunctfdf2` / accumulator materialization for `F128`, in [ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:457).
- The supporting helper surface needed for that route is already real in-tree instead of requiring a new family activation: `emit_i128_store_result_impl(...)` exists in [src/backend/riscv/codegen/i128_ops.cpp](/workspaces/c4c/src/backend/riscv/codegen/i128_ops.cpp:374), and the RV64 F128 hooks already expose result truncation / paired-store helpers in [src/backend/riscv/codegen/f128.cpp](/workspaces/c4c/src/backend/riscv/codegen/f128.cpp:123).
- The current proof lane for calls result-store only covers non-`F128` cases, which matches the remaining gap and keeps this continuation honest rather than speculative: [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1289).

### 3. The other visible families are broader than this continuation and should stay parked at this checkpoint.

- `variadic.cpp` is not yet a real owner surface; it currently exposes only local arithmetic helpers plus comments naming future method boundaries, while the shared header does not declare any `emit_va_*_impl(...)` entrypoints or state needed for them: [src/backend/riscv/codegen/variadic.cpp](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:30), [src/backend/riscv/codegen/riscv_codegen.hpp](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:318), [src/backend/riscv/codegen/riscv_codegen.hpp](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:510). The Rust reference needs `va_named_gp_count`, `va_named_stack_bytes`, and three owner methods, so taking `variadic.cpp` next would require a broader surface activation, not a narrower continuation: [ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs:42).
- `memory.cpp` already has the scalar/F128 direct, indirect, and over-aligned load/store seams that were selected earlier; any next step there is the broader typed-indirect / GEP owner work the plan explicitly parks, not a smaller follow-on packet: [src/backend/riscv/codegen/memory.cpp](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:77), [plan.md](/workspaces/c4c/plan.md:68).
- `comparison.cpp` already landed the selected `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` route, and the plan explicitly parks any broader comparison expansion beyond that seam: [src/backend/riscv/codegen/comparison.cpp](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:191), [plan.md](/workspaces/c4c/plan.md:70).

## Judgments

- Plan-alignment judgment: `on track`
- Technical-debt judgment: `watch`
- Route judgment: the current blocked checkpoint is still correct, but the next truthful broader lifecycle route is no longer inside aggregate / struct outgoing-call staging.

## Route Decision

- Selected next broader lifecycle route: broader translated `calls.cpp` result-store expansion centered on `emit_call_store_result_impl(...)` for the parked `I128` and `F128` return paths.
- Narrowest honest continuation from the blocked checkpoint: one bounded calls-owner packet that makes the `I128` paired-result store real and wires the `F128` paired-result store plus truncate-to-accumulator handoff through the existing RV64 helper surface and focused proof lane.
- Parked families:
  - keep `variadic.cpp` parked until the shared RV64 header/state surface can truthfully host `emit_va_arg_impl(...)`, `emit_va_start_impl(...)`, and `emit_va_copy_impl(...)`
  - keep broader `memory.cpp` typed-indirect and broader GEP owner work parked
  - keep broader comparison follow-on beyond `emit_fused_cmp_branch_impl(...)` / `emit_select_impl(...)` parked
  - do not reopen the exhausted aggregate / struct outgoing-call family as another direct packet

## Recommendation

- Reviewer recommendation: `rewrite plan/todo before more execution`
- Rationale: the branch is correctly blocked at `77482587`, and the next truthful route is now a different broader calls family than the completed aggregate / struct outgoing-call lane. The supervisor should hand this report to the plan owner so lifecycle state can be rewritten to the parked `I128` / `F128` calls result-store route before any executor packet is issued.
