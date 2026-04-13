# RV64 Post-ALU-Unary Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `5932d490`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to alu unary helper seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit naming the active packet, and the
  only execution diff on top of it is the just-landed bounded unary-helper
  seam at `HEAD`
- commits since base: `1`

## Validation Observed

- reviewed the implementation delta for `src/backend/riscv/codegen/alu.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`, `CMakeLists.txt`, and
  `tests/backend/backend_shared_util_tests.cpp`
- reviewed proof evidence from `rv64_alu_unary_build.log`,
  `rv64_alu_unary_shared_util.log`, and
  `rv64_alu_unary_header_shared_util.log`
- observed that the build log shows both `c4cll` and
  `backend_shared_util_tests` rebuilt with `alu.cpp`, and both filtered
  shared-util logs are empty because the targeted test runs passed without
  emitting failure text

## Findings

- Medium: canonical lifecycle state now points at a packet that `HEAD` already
  landed. `plan.md` and `todo.md` still name the translated `alu.cpp`
  unary-helper seam as the next packet even though commit `ac795f01` already
  wired it into the shared header, build lists, and focused shared-util
  coverage
- Medium: the landed slice itself remains bounded. The real owner behavior is
  limited to `emit_float_neg_impl(...)`, `emit_int_neg_impl(...)`, and
  `emit_int_not_impl(...)`, with only the expected shared-header exports,
  source-list wiring, and focused shared-util assertions
- Medium: there is not yet a new bounded follow-on packet chosen from the live
  surface. The remaining visible `alu.cpp` owner bodies still depend on the
  broader operand/result helpers needed for integer binops and i128 copy, while
  the previously parked `memory.cpp`, `globals.cpp`, and `calls.cpp` families
  remain wider than the just-landed unary lane

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not issue another execution packet from the stale lifecycle state.

Rewrite `plan.md` / `todo.md` first so canonical state:

- marks the bounded translated `alu.cpp` unary-helper seam as complete
- records the build and focused shared-util proof logs for that slice
- blocks further execution until a fresh post-alu owner-file audit selects the
  next truly bounded RV64 seam

## Rationale

The branch has not drifted. The implementation commit did exactly what the
latest lifecycle checkpoint asked for, and the focused proof evidence matches
that bounded slice. The remaining work is route selection, not implementation
repair, so the correct next action is a lifecycle rewrite rather than guessing
another packet from stale plan state.
