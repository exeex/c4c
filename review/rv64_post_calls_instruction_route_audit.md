# RV64 Post-Calls-Instruction Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `9c401cd0`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 calls lane pending post-instruction audit`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and `HEAD`
  has not drifted past that block-before-routing checkpoint
- commits since base: `0`

## Scope Reviewed

- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/emit.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/{calls,globals,emit}.rs`

## Findings

- Medium: the just-landed `calls.cpp` instruction / cleanup seam is complete
  and still bounded, but the remaining visible `calls.cpp` inventory is not the
  next narrow packet. The only untranslated methods left in the live C++ file
  are `emit_call_f128_pre_convert_impl(...)`,
  `emit_call_stack_args_impl(...)`, `emit_call_reg_args_impl(...)`, and
  `emit_call_store_result_impl(...)`, and those bodies pull in wider stack
  staging, register-assignment, F128 materialization, and result-tracking
  surface than the current shared RV64 header exposes
  ([`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:55),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:25),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:457)).
- Medium: the remaining `alu.cpp` and `memory.cpp` owner bodies are still
  broader than the helper-sized lane that has been landing so far. The live
  C++ files already document that the visible next work in those families still
  depends on wider operand/result or shared-state surface, so they are not a
  smaller follow-on than the completed calls instruction / cleanup seam
  ([`src/backend/riscv/codegen/alu.cpp`](/workspaces/c4c/src/backend/riscv/codegen/alu.cpp:27),
  [`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:204)).
- Medium: the smallest practical next seam is no longer in the remaining
  `calls.cpp` body, but in the shared result-store helper that the translated
  RV64 tree already expects: `store_t0_to(...)`. The Rust reference keeps that
  helper in `emit.rs`, and both `globals.rs` and later call-result work depend
  on it; the current C++ shared header has `emit_store_to_s0(...)` and
  `operand_to_t0(...)` declarations but still lacks any shared result-store
  helper
  ([`ref/claudes-c-compiler/src/backend/riscv/codegen/emit.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/emit.rs:362),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:304)).
- Medium: once `store_t0_to(...)` exists on the shared RV64 surface, the
  narrowest translated owner follow-on is `globals.cpp::emit_label_addr_impl(...)`,
  not broader call lowering and not GOT-dependent global address loading.
  `emit_label_addr_impl(...)` only emits `lla t0, <label>` and stores the
  result, while `emit_global_addr_impl(...)` still depends on the missing
  `state.needs_got(...)` query and broader globals routing should remain parked
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:25),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:9),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:20)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not route the next packet back into the remaining broad `calls.cpp` bodies.

Rewrite `plan.md` / `todo.md` first so canonical state:

- marks the bounded translated `calls.cpp` instruction / cleanup seam as
  complete with the existing build and focused shared-util proof lane
- records that the next bounded RV64 packet is a preparatory shared helper plus
  a one-method globals seam:
  add the shared `store_t0_to(...)` result-store helper on the RV64 C++ surface
  and wire only `RiscvCodegen::emit_label_addr_impl(...)`
- keeps `emit_global_addr_impl(...)` parked until a later route can justify the
  missing `needs_got(...)` surface

Keep these families parked:

- `emit_call_f128_pre_convert_impl(...)`
- `emit_call_stack_args_impl(...)`
- `emit_call_reg_args_impl(...)`
- `emit_call_store_result_impl(...)`
- remaining `alu.cpp` integer-binop and i128-copy bodies
- broader `memory.cpp` owner work beyond the landed helper seams
- `globals.cpp::emit_global_addr_impl(...)`
- `globals.cpp::emit_tls_global_addr_impl(...)` unless the lifecycle rewrite
  explicitly couples it to the same result-store seam later

## Rationale

The branch has not drifted; this is a pure route-selection audit. The completed
calls instruction / cleanup slice exhausted the only remaining narrow call body
that fit the already-shared operand/emission surface. The next reusable RV64
boundary exposed by the reference implementation is not another call helper but
the shared `store_t0_to(...)` path in `emit.rs`. Routing that preparatory
helper directly into `globals.cpp::emit_label_addr_impl(...)` preserves the
integration-first lane, advances a real translated owner file, and avoids
reopening the wider call-result, stack-arg, or GOT-state surfaces too early.
