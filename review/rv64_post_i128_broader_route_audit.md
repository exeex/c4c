# RV64 Post-I128 Broader Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `59630754`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending broader i128 route`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle checkpoint for the active plan, and
  `HEAD` has not moved past that blocked broader-route state
- commits since base: `0`

## Scope Reviewed

- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/i128_ops.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/{alu,comparison,calls,globals}.rs`
- `tests/backend/backend_shared_util_tests.cpp`

## Validation Checked

- `scripts/plan_change_gap.sh`
- `git log --oneline --grep='\[plan_change\]' -- plan.md todo.md ideas/open/`
- `git diff 59630754..HEAD`

No new build was run for this review-only checkpoint.

## Findings

- Medium: the smallest truthful broader follow-on is now the translated
  `alu.cpp` integer-owner seam, not another i128 packet or a pivot to a wider
  family. The live RV64 ALU file stops after the already-landed unary helpers
  and says the remaining integer-binop and i128-copy bodies are still parked
  behind a broader surface
  (`src/backend/riscv/codegen/alu.cpp:27-31`), but the Rust inventory shows
  that the remaining owner work is only two contiguous methods:
  `emit_int_binop_impl(...)` and `emit_copy_i128_impl(...)`
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/alu.rs:30-76`). The
  broader i128 lane that previously blocked this route is now real in-tree:
  pair load/store helpers, binop prep, arithmetic/logical ops, shifts,
  div/rem-call lowering, and i128 compare helpers are already present in
  `src/backend/riscv/codegen/i128_ops.cpp:59-467`. That leaves ALU as the next
  adjacent consumer route rather than another fresh shared-surface invention.
- Medium: broader comparison owner work is still wider than the ALU follow-on.
  The live C++ comparison file still contains only the bounded float-compare
  path (`src/backend/riscv/codegen/comparison.cpp:1-4`,
  `src/backend/riscv/codegen/comparison.cpp:56-87`), while the remaining Rust
  inventory requires compare-operand loading, fresh labels, branch emission,
  and register-cache invalidation across integer compare, fused compare+branch,
  and select (`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:36-135`).
  The current shared RV64 state surface does not carry label-generation or
  reg-cache machinery (`src/backend/riscv/codegen/riscv_codegen.hpp:262-303`),
  so comparison is still a broader route than the next ALU packet.
- Medium: broader outgoing-call lowering remains materially wider than the
  blocked checkpoint and should stay parked. The live C++ file explicitly says
  the surviving call-owner methods still depend on a wider shared
  `RiscvCodegen` / `CodegenState` surface
  (`src/backend/riscv/codegen/calls.cpp:121-131`), and the Rust follow-on
  inventory immediately fans out into F128 pre-convert, stack-arg lowering,
  register-arg staging, struct cases, and paired I128/F128 argument handling
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:29-220`).
- Low: TLS globals is still too isolated to be the next truthful route. The
  live C++ file keeps only the already-landed global and label address helpers
  and explicitly notes that broader globals work still depends on shared GOT
  and TLS state outside the current bounded RV64 surface
  (`src/backend/riscv/codegen/globals.cpp:48-65`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state routes
the next lifecycle stage to the broader translated `src/backend/riscv/codegen/alu.cpp`
integer-owner seam, with only the minimum shared RV64 surface additions needed
to make that owner packet real.

The first proof-bearing packet on that route should stay centered on:

- `IrBinOp` exposure through `src/backend/riscv/codegen/riscv_codegen.hpp`
- `RiscvCodegen::emit_int_binop_impl(...)`
- `RiscvCodegen::emit_copy_i128_impl(...)`
- any strictly required declaration/build wiring for those two owner entries

Expected focused proof lane for that route:

- `cmake --build build --target backend_shared_util_tests c4cll`
- a new declaration-reachability check for the broadened RV64 ALU owner surface
- a bounded emitted-text test that proves integer add/sub/mul/div/rem,
  bitwise ops, shifts, and i128-copy lowering through the translated RV64 ALU
  owner without widening into comparison or call lowering

Keep these families parked:

- broader `comparison.cpp` integer compare, fused-branch, and select owner work
- broader `calls.cpp` outgoing-call lowering plus parked I128 / F128
  argument/result paths
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `memory.cpp` typed-indirect and broader GEP owner work

## Rationale

The blocked post-i128 checkpoint was correct: there is no equally narrow
helper-only packet left in `i128_ops.cpp`. But the route does not need a wide
reset into calls, comparison, or TLS. The just-landed i128 owner family now
provides the exact adjacent support surface that the parked ALU owner note was
waiting on, and the remaining Rust ALU inventory is compact enough to route as
one truthful broader packet.
