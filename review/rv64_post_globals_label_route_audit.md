# RV64 Post-Globals-Label Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `fac97d98`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to globals label-address seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only commit since then is the landed implementation slice for that exact
  packet
- commits since base: `1`
  (`b255cfd2 riscv: wire translated globals label-address seam`)

## Scope Reviewed

- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/returns.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/riscv/codegen/float_ops.cpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `CMakeLists.txt`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/{globals,float_ops,emit}.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `cmake --build build --target c4cll`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_globals_label_surface`
- `./build/backend_shared_util_tests riscv_translated_globals_label_helper_emits_bounded_text`

All four commands passed during this review.

## Findings

- Medium: the landed globals packet is aligned with the active route and stays
  bounded. `emit_label_addr_impl(...)` now exists as a single translated owner
  body, the shared RV64 surface gained only the expected declaration, and the
  preparatory `store_t0_to(...)` helper is limited to register-or-stack result
  placement rather than wider globals or call-result behavior
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:48),
  [`src/backend/riscv/codegen/returns.cpp`](/workspaces/c4c/src/backend/riscv/codegen/returns.cpp:68),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:300),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:371),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:831)).
- Medium: the remaining visible `globals.cpp` bodies are not the next narrow
  packet. `emit_global_addr_impl(...)` still depends on a missing
  `state.needs_got(...)` query, and `emit_tls_global_addr_impl(...)` still
  requires TLS relocation/state handling that is nowhere on the shared C++
  RV64 surface yet, so continuing inside `globals.cpp` would widen the lane
  into backend-state work rather than another helper-sized owner seam
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:14),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:34),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:7),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:27)).
- Medium: now that `store_t0_to(...)` is shared, the narrowest practical
  follow-on is a float-owner packet rather than more globals work. The live
  `float_ops.cpp` file already contains the local mnemonic/body helpers, and
  the Rust owner entry only needs shared `FloatOp`, `operand_to_t0(...)`,
  `store_t0_to(...)`, and `IrType` routing for the non-`F128` path, while the
  comparison and variadic files still imply additional shared state or helper
  surface
  ([`src/backend/riscv/codegen/float_ops.cpp`](/workspaces/c4c/src/backend/riscv/codegen/float_ops.cpp:14),
  [`src/backend/riscv/codegen/float_ops.cpp`](/workspaces/c4c/src/backend/riscv/codegen/float_ops.cpp:62),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs:9),
  [`src/backend/x86/codegen/x86_codegen.hpp`](/workspaces/c4c/src/backend/x86/codegen/x86_codegen.hpp:124)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `watch`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks the preparatory shared `store_t0_to(...)` helper plus
  `globals.cpp::emit_label_addr_impl(...)` seam complete with the focused
  build/test proof lane above
- records that the remaining `globals.cpp` bodies stay parked because they
  require missing GOT/TLS state
- routes the next bounded RV64 packet to `float_ops.cpp`:
  wire the translated non-`F128` `emit_float_binop_impl(...)` owner entry and
  the minimal shared `FloatOp` / helper declaration surface it needs, while
  keeping `emit_f128_neg_impl(...)` and broader floating-point expansion parked

Keep these families parked:

- `globals.cpp::emit_global_addr_impl(...)`
- `globals.cpp::emit_tls_global_addr_impl(...)`
- remaining `alu.cpp` integer-binop and i128-copy bodies
- broader `memory.cpp` owner work
- broader call-result lowering beyond the landed instruction / cleanup seam
- comparison, variadic, atomics, and intrinsic owner packets unless a later
  audit shows one is narrower than the float binop route

## Rationale

The branch is still aligned with the plan: the single commit after the latest
lifecycle checkpoint lands exactly the globals label-address packet that the
runbook named, and the focused proof lane passes. The route is stale only
because canonical lifecycle state still points at a slice that is already in
`HEAD`.

At this checkpoint the globals lane is exhausted for bounded work. The two
remaining translated globals bodies both require backend-state expansion that
the current RV64 C++ seam still does not expose. In contrast, `float_ops.cpp`
already has most of its local translation work present, and the next owner
entry can reuse the newly shared `store_t0_to(...)` helper without first
opening GOT/TLS or broader call/memory state.
