# RV64 Post-Calls-ABI Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `6322da18`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to calls abi helper seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit naming the active packet, and the
  only execution diff on top of it is the just-landed bounded calls ABI-helper
  seam at `HEAD`
- commits since base: `1`

## Validation Observed

- reviewed the implementation delta for `src/backend/riscv/codegen/calls.cpp`,
  `src/backend/riscv/codegen/riscv_codegen.hpp`, `CMakeLists.txt`, and
  `tests/backend/backend_shared_util_tests.cpp`
- reviewed proof evidence from `rv64_calls_build.log`,
  `rv64_calls_header_shared_util.log`, and `rv64_calls_shared_util.log`
- observed that the build log shows both requested targets rebuilt from a clean
  graph state (`ninja: no work to do`), and both filtered shared-util logs are
  empty because the targeted tests passed without emitting failure text

## Findings

- Medium: canonical lifecycle state now points at a packet that `HEAD` already
  landed. `plan.md` and `todo.md` still name the standalone translated
  `calls.cpp` ABI-helper seam as the next packet even though commit `eab80f1d`
  already wired `CallAbiConfig`, `CallArgClass`,
  `call_abi_config_impl()`, and
  `emit_call_compute_stack_space_impl(...)` into the shared RV64 surface,
  active build lists, and focused shared-util coverage.
- Medium: the landed slice remains bounded. The real owner behavior is limited
  to the ABI config plus stack-space computation in
  `src/backend/riscv/codegen/calls.cpp`, with only the expected shared-header
  exports, source-list wiring, and focused shared-util assertions on top.
- Medium: unlike the earlier post-alu state, there is a visible next bounded
  follow-on in the same owner file. The remaining `calls.rs` inventory includes
  `emit_call_instruction_impl(...)` and `emit_call_cleanup_impl(...)`, and both
  depend only on already-shared `Operand`, basic scalar parameters, and
  `RiscvCodegen::state.emit` / `operand_to_t0(...)`; they do not require the
  wider shared state still missing for stack-arg materialization, register-arg
  setup, F128 pre-conversion, or result storage.
- Medium: broader `calls.cpp` lowering is still not ready to route. The
  remaining `emit_call_f128_pre_convert_impl(...)`,
  `emit_call_stack_args_impl(...)`, `emit_call_reg_args_impl(...)`, and
  `emit_call_store_result_impl(...)` bodies still imply missing stack helpers,
  register-assignment/state hooks, and result-store surfaces that would widen
  the current lane.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not issue another execution packet from the stale lifecycle state.

Rewrite `plan.md` / `todo.md` first so canonical state:

- marks the bounded translated `calls.cpp` ABI-helper seam as complete
- records the build and focused shared-util proof logs for that slice
- routes the next execution packet to the bounded translated
  `calls.cpp` call-instruction / cleanup seam only:
  `emit_call_instruction_impl(...)` plus `emit_call_cleanup_impl(...)`

Keep these families parked:

- `emit_call_f128_pre_convert_impl(...)`
- `emit_call_stack_args_impl(...)`
- `emit_call_reg_args_impl(...)`
- `emit_call_store_result_impl(...)`
- remaining `alu.cpp` integer-binop and i128-copy bodies
- broader `memory.cpp` owner work beyond the landed helper seams
- `globals.cpp` owner work

## Rationale

The branch has not drifted. The landed calls ABI-helper packet did exactly what
the latest lifecycle checkpoint asked for, and the focused proof evidence
matches that bounded slice. Unlike the earlier post-alu state, the remaining
live `calls.rs` inventory still exposes a practical next helper seam: direct
and indirect call instruction emission plus stack cleanup stay narrowly within
the existing shared `Operand` surface and emission path. The wider call-arg and
result-store methods still need missing backend state, so canonical lifecycle
state should route specifically to the instruction/cleanup seam rather than
block for another broad owner-file audit.
