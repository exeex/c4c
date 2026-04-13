# RV64 Post-Global-Address Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `358998b6`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane to globals got-aware global-address seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan, and the
  only execution diff since then is the landed implementation slice for that
  exact packet
- commits since base: `1`
  (`9e62b2f0 rv64: wire GOT-aware global address seam`)

## Scope Reviewed

- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/returns.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/riscv/codegen/float_ops.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/f128.cpp`
- `src/backend/f128_softfloat.cpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `ref/claudes-c-compiler/src/backend/{f128_softfloat,riscv/codegen/{float_ops,f128,comparison,memory,calls,globals}}.rs`

## Validation Checked

- `cmake --build build --target backend_shared_util_tests`
- `cmake --build build --target c4cll`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_global_addr_surface`
- `./build/backend_shared_util_tests riscv_translated_globals_global_addr_helper_emits_bounded_text`

All four commands passed at this checkpoint.

## Findings

- Medium: the landed globals packet is aligned with the active route and stays
  bounded. The shared RV64 surface gained only the expected GOT-address query
  state plus the translated `emit_global_addr_impl(...)` owner entry, and the
  focused shared-util coverage proves both the local `lla` path and the
  GOT-aware `la` path without widening into TLS handling
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:48),
  [`src/backend/riscv/codegen/returns.cpp`](/workspaces/c4c/src/backend/riscv/codegen/returns.cpp:39),
  [`src/backend/riscv/codegen/riscv_codegen.hpp`](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:259),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:453),
  [`tests/backend/backend_shared_util_tests.cpp`](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:1073)).
- Medium: the remaining visible globals follow-on is not another honest narrow
  packet. `emit_tls_global_addr_impl(...)` still depends on TLS relocation and
  state handling that the shared RV64 seam does not model yet, so continuing
  inside `globals.cpp` would hide a wider state expansion inside another owner
  packet
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:34)).
- Medium: the broader non-globals follow-on families remain blocked on wider
  state than the current seam exposes. `memory.cpp` still widens into typed
  indirect and F128 paths, `calls.cpp` still widens into broader outgoing-call
  lowering, and `comparison.cpp` still widens into label/branch-cache or F128
  state rather than another helper-sized entry
  ([`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:58),
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:90),
  [`src/backend/riscv/codegen/comparison.cpp`](/workspaces/c4c/src/backend/riscv/codegen/comparison.cpp:56)).
- Medium: the smallest truthful broader route is now the shared F128
  soft-float seam, not another one-method owner packet. The RV64 translated
  files already point at `backend/f128_softfloat.rs` for `emit_f128_neg_impl`,
  F128 compare, and F128 memory follow-ons, while the live C++ mirror file is
  still only a stub; that makes the missing shared soft-float orchestration a
  concrete integration boundary rather than speculative future work
  ([`src/backend/f128_softfloat.cpp`](/workspaces/c4c/src/backend/f128_softfloat.cpp:1),
  [`src/backend/riscv/codegen/f128.cpp`](/workspaces/c4c/src/backend/riscv/codegen/f128.cpp:3),
  [`ref/claudes-c-compiler/src/backend/f128_softfloat.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/f128_softfloat.rs:412),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/f128.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/f128.rs:347),
  [`ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs`](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/float_ops.rs:50)).
- Low: broad `ctest -R '^backend_shared_util_tests$'` remains red on this
  branch because of an unrelated existing x86 unsigned-float-helper failure, so
  this review used the focused RV64 globals proof lane above rather than
  treating the unrelated broad-suite red as a blocker for the landed globals
  slice
  ([`rv64_globals_after.log`](/workspaces/c4c/rv64_globals_after.log:1)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- marks the bounded RV64 GOT-aware `globals.cpp::emit_global_addr_impl(...)`
  seam complete with the focused build/test proof lane above
- records that `emit_tls_global_addr_impl(...)` stays parked because it still
  needs wider TLS relocation/state handling
- routes the next lifecycle stage to a broader shared-surface expansion:
  define the shared `backend/f128_softfloat.cpp` orchestration seam plus the
  minimal RV64 hook surface needed to make
  `src/backend/riscv/codegen/f128.cpp::emit_f128_neg_full(...)` and then
  `src/backend/riscv/codegen/float_ops.cpp::emit_f128_neg_impl(...)`
  truthful follow-ons

Keep these families parked:

- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `memory.cpp` F128 / typed-indirect / wider GEP follow-on work
- `comparison.cpp::emit_int_cmp_impl(...)`
- `comparison.cpp::emit_fused_cmp_branch_impl(...)`
- `comparison.cpp::emit_select_impl(...)`
- `comparison.cpp::emit_f128_cmp_impl(...)`
- broader `calls.cpp` outgoing-call lowering and F128/I128 result-store work
- `alu.cpp` integer-binop and i128-copy owner bodies
- `variadic.cpp`

## Rationale

The branch is still aligned with the plan: the single commit after the latest
lifecycle checkpoint lands exactly the globals global-address packet that the
runbook named, and the focused proof lane passes. The route is stale only
because canonical lifecycle state still points at a packet that is already in
`HEAD`.

At this checkpoint the previously productive helper-sized lane is exhausted
again. The remaining visible globals, memory, comparison, call, and integer
ALU follow-ons all imply materially wider state than the current RV64 surface
models. The most concrete shared gap now visible across the translated RV64
inventory is the missing `backend/f128_softfloat.cpp` orchestration seam: the
reference routes multiple parked RV64 F128 owners through it, while the live
C++ file is still only a stub. That makes the next truthful lifecycle move a
broader shared-surface route rather than another one-method owner packet.
