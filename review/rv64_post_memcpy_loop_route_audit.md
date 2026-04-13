# RV64 Post-Memcpy-Loop Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `080afddf`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to memcpy loop helper seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit that named the current active
  packet, and the only execution diff on top of it is the just-landed bounded
  memcpy-loop helper seam at `HEAD`
- commits since base: `1`

## Validation Observed

- reviewed `git diff 080afddf..HEAD` for the landed execution slice
- reviewed proof evidence from `rv64_memcpy_loop_build.log` and
  `rv64_memcpy_loop_shared_util.log`
- observed that the build log shows successful rebuilds of `c4cll` and
  `backend_shared_util_tests`, and the focused shared-util log is empty because
  the filtered test command passed without emitting output

## Findings

- Medium: canonical lifecycle state now points at work that `HEAD` already
  landed. `plan.md` still names `emit_memcpy_impl_impl(...)` as the next
  bounded packet, and `todo.md` still lists the same step as pending, but the
  implementation is now real at
  `src/backend/riscv/codegen/memory.cpp:196`-`210`, the shared declaration is
  exported at `src/backend/riscv/codegen/riscv_codegen.hpp:167`-`169`, and the
  focused coverage now reaches that seam at
  `tests/backend/backend_shared_util_tests.cpp:273`-`312` and
  `tests/backend/backend_shared_util_tests.cpp:544`-`620`. More execution
  should not start until `plan.md` / `todo.md` advance past this completed
  packet.
- Medium: the landed slice itself remains aligned and narrow. The new owner
  body is limited to the translated byte-copy loop at
  `src/backend/riscv/codegen/memory.cpp:196`-`210`, with only the expected
  shared-header reachability addition at
  `src/backend/riscv/codegen/riscv_codegen.hpp:169` and matching focused
  shared-util assertions at
  `tests/backend/backend_shared_util_tests.cpp:601`-`620`. I did not find
  widening into default load/store delegation, typed indirect access, broader
  GEP bodies, or call ABI work.
- Medium: the remaining visible `memory.cpp` route is still wider than the
  helper lane that just completed. The parked translated owner bodies starting
  at `src/backend/riscv/codegen/memory.cpp:223` still depend on unresolved
  broader surfaces such as default load/store delegation, slot-address
  resolution, type-based load/store selection, and wider shared RV64 state.
  Based on the current live surface, there is no equally small next helper in
  `memory.cpp` that is as isolated as the completed memcpy loaders and memcpy
  loop.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not continue execution from the stale lifecycle state, and do not guess at
another `memory.cpp` helper packet.

Rewrite `plan.md` / `todo.md` first so the next bounded route is explicit.
From the current reviewed surface, that rewrite should:

- mark the bounded `memory.cpp::emit_memcpy_impl_impl(...)` seam as complete
- stop treating `memory.cpp` as the automatic next packet source unless a fresh
  owner-file audit identifies another truly bounded seam
- keep the broader parked families parked:
  default load/store delegation, typed indirect memory-owner bodies, broader
  GEP owner bodies, broader shared RV64 state expansion, and call ABI work

## Rationale

The branch has not drifted. The current implementation commit does exactly what
the latest lifecycle checkpoint asked for, and the focused proof evidence is
consistent with that narrow slice. The problem is now canonical routing, not
implementation quality: the active runbook still points at a packet that has
already landed, while the remaining commented `memory.cpp` bodies are visibly
broader than the helper lane the plan has been following. That makes lifecycle
rewrite the right next action before any more execution packets are issued.
