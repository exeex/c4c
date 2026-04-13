# RV64 Post-ALU Owner-File Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `ab44c554`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending post-alu route audit`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the current active plan, and
  `HEAD` has not drifted past that route-reset checkpoint
- commits since base: `0`

## Scope Reviewed

- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/{alu,globals,calls}.rs`

## Findings

- Medium: the remaining visible `alu.cpp` body is still not a bounded next
  seam. The live file explicitly stops after the landed unary helpers and notes
  that the remaining integer-binop and i128-copy bodies still depend on the
  broader operand/result surface that is not yet shared in
  [`src/backend/riscv/codegen/alu.cpp`](/workspaces/c4c/src/backend/riscv/codegen/alu.cpp:27).
- Medium: `globals.cpp` is still wider than the current lane. It remains a
  comment-only structural mirror and still depends on missing shared state such
  as `state.needs_got(...)` plus result-store hooks, so it is not a smaller
  follow-on than the just-landed unary helper seam
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:2),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:12)).
- Medium: the smallest live bounded follow-on is in `calls.cpp`, but only for
  the standalone ABI helper surface, not for broader call lowering. The file
  already contains self-contained translated ABI types and stack-sizing logic
  (`CallAbiConfig`, `CallArgClass`, `compute_stack_arg_space(...)`, and
  `call_abi_config_impl()`) in
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:91),
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:106),
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:254),
  while the same file explicitly marks the broader call-owner methods as still
  requiring the wider shared `RiscvCodegen` / `CodegenState` surface in
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:286).
- Medium: the current shared RV64 header has no call-ABI surface yet, but it
  is still narrow enough to add just the ABI structs and the two bounded entry
  points needed for the helper seam without reopening broader call lowering
  ([`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:113)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

The next bounded execution packet should move into `src/backend/riscv/codegen/calls.cpp`,
but only for the standalone ABI-helper seam:

- shared `CallAbiConfig`, `CallArgClass`, and any strictly required support
  types
- `RiscvCodegen::call_abi_config_impl()`
- `RiscvCodegen::emit_call_compute_stack_space_impl(...)` backed by the
  standalone stack-sizing helper
- minimal build wiring and focused header-export / shared-util coverage for
  that helper-only surface

Keep these families parked:

- remaining `alu.cpp` integer-binop and i128-copy bodies
- broader `memory.cpp` owner work beyond the landed helper seams
- `globals.cpp` owner work
- broader `calls.cpp` lowering (`emit_call_stack_args_impl(...)`,
  `emit_call_reg_args_impl(...)`, instruction emission, cleanup, and result
  storage)

## Rationale

The lane does not need another broad route reset. The post-alu audit shows a
live translated owner file that already contains a practical standalone helper
surface, and it is narrower than the still-blocked alternatives. `calls.cpp`
already isolates ABI configuration and stack-space computation from the wider
output/state hooks, so it is the smallest follow-on packet that advances the
shared RV64 surface without inventing the broader call backend API.
