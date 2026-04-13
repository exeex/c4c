# RV64 Post-Default-Cast Broader-Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `d9762e56`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane after shared default-cast seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle checkpoint for the active plan after the
  shared default-cast / i128-helper seam landed; the only later commit is the
  routed skill-frontmatter repair and does not change RV64 route intent
- commits since base: `1`
  (`b1cda4b9 skills: quote c4c role descriptions`)

## Scope Reviewed

- `src/backend/riscv/codegen/i128_ops.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/i128_ops.rs`
- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/comparison.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs`
- `src/backend/riscv/codegen/calls.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs`
- `src/backend/riscv/codegen/globals.cpp`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `tests/backend/backend_shared_util_tests.cpp`

## Validation Checked

- `scripts/plan_change_gap.sh`
- `git log --oneline --grep='\[plan_change\]' -- plan.md todo.md ideas/open/`
- `git diff d9762e56..HEAD`
- existing focused RV64 cast proof references in
  `tests/backend/backend_shared_util_tests.cpp:305-334`
- existing x86 translated i128 owner proof shape in
  `tests/backend/backend_shared_util_tests.cpp:1964-2052`

No new build was run for this review-only checkpoint.

## Findings

- Medium: the smallest truthful broader follow-on is now the translated
  `i128_ops.cpp` owner family, not another cast packet.
  `src/backend/riscv/codegen/i128_ops.cpp:1-3` explicitly says the current C++
  file only wires the helper surface needed by the shared default-cast seam and
  parks the broader translated i128 owner work. The Rust source keeps the next
  unresolved owner inventory concentrated in that same file:
  pair slot/indirect helpers, saved-pair staging, unary neg/not, binop prep,
  arithmetic/logical/shift primitives, div/rem call lowering, result-store,
  and compare helpers
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/i128_ops.rs:19-219` and
  `:250-351`). That makes broader i128 the most contiguous next lane that still
  stays adjacent to the just-landed default-cast / helper seam.
- Medium: the remaining `calls.cpp` route is broader than the current blocked
  checkpoint and should stay parked.
  `src/backend/riscv/codegen/calls.cpp:121-131` already marks the surviving
  translated call methods as dependent on a wider shared `RiscvCodegen` /
  `CodegenState` surface. The Rust follow-on inventory spans F128 pre-convert,
  stack-arg emission, register-arg emission, struct cases, split register/stack
  cases, and paired I128/F128 argument handling
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/calls.rs:29-437`), which
  is a materially wider integration step than broadening `i128_ops.cpp`.
- Medium: comparison/select is still a larger cross-cutting route than broader
  i128.
  `src/backend/riscv/codegen/comparison.cpp:1-4` keeps only the bounded
  float-compare seam active today. The remaining Rust owner bodies depend on
  missing compare-operand loading, label generation, control-flow emission, and
  register-cache invalidation across integer compare, fused compare+branch, and
  select (`ref/claudes-c-compiler/src/backend/riscv/codegen/comparison.rs:36-136`).
  That is a wider shared-surface expansion than the next i128-centered route.
- Medium: TLS globals remains too isolated and state-heavy to be the next
  broader route.
  `src/backend/riscv/codegen/globals.cpp:62-63` already records that the
  remaining globals owner work depends on shared GOT/TLS state not present in
  the bounded RV64 surface. The Rust TLS path is only one method, but it is
  relocation- and runtime-model-specific
  (`ref/claudes-c-compiler/src/backend/riscv/codegen/globals.rs:27-35`) and
  does not unlock the broader cast/i128 lane that is now immediately adjacent.
- Low: `alu.cpp` integer-binop / i128-copy is a consumer of the broader i128
  route, not a better first pivot.
  `src/backend/riscv/codegen/alu.cpp:27-31` explicitly says the remaining
  translated integer binop and i128-copy bodies still depend on a broader
  shared operand/result surface. That makes ALU follow-on more honest after the
  i128 owner lane is selected, not before.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Rewrite `plan.md` / `todo.md` before more execution so canonical state:

- accepts the blocked post-default-cast checkpoint in
  `review/rv64_post_default_cast_route_audit.md`
- routes the next lifecycle stage to the broader translated
  `src/backend/riscv/codegen/i128_ops.cpp` owner family plus only the minimum
  shared `src/backend/riscv/codegen/riscv_codegen.hpp` exports and adjacent
  helper hooks needed to make that owner surface real
- uses the existing x86 translated i128 owner proof shape as the packet model
  instead of inventing a new RV64-only route shape

The first proof-bearing packet on that broader route should stay centered on:

- `emit_store_pair_to_slot_impl(...)`
- `emit_load_pair_from_slot_impl(...)`
- `emit_save_acc_pair_impl(...)`
- `emit_store_pair_indirect_impl(...)`
- `emit_load_pair_indirect_impl(...)`
- `emit_i128_neg_impl(...)`
- `emit_i128_not_impl(...)`
- `emit_i128_prep_binop_impl(...)`
- `emit_i128_add_impl(...)`
- `emit_i128_store_result_impl(...)`
- `emit_i128_cmp_ordered_impl(...)`
- `emit_i128_cmp_store_result_impl(...)`

Expected focused proof lane for that route:

- `cmake --build build --target backend_shared_util_tests c4cll`
- a new declaration-reachability check for the broadened RV64 i128 header
  surface
- a bounded emitted-text test modeled on the existing x86 translated i128 owner
  helper proof

Keep these families parked:

- broader i128 shift/divrem families beyond the first proof-bearing packet
- broader `calls.cpp` outgoing-call lowering and I128/F128 argument/result work
- `comparison.cpp` integer compare, fused branch, and select owner work
- `globals.cpp::emit_tls_global_addr_impl(...)`
- broader `memory.cpp` typed-indirect and broader GEP owner work
- broader `alu.cpp` integer-binop and i128-copy owner work until the chosen
  i128 route lands enough shared surface to make them truthful

## Rationale

The shared default-cast seam consumed the last honest cast-local packet. The
next real choice is between broader adjacent families, and the translated RV64
tree already tells us which one is most self-contained: `i128_ops.rs` keeps a
large, contiguous owner surface in one file, while the surviving calls,
comparison, and TLS globals work each fan out into wider shared-surface
dependencies immediately.

Choosing broader i128 now also preserves the integration-first pattern already
used in this repo: advance to the next truthful owner family nearest the
landed seam, keep proof focused, and do not widen into unrelated ABI, control
flow, or relocation work just because those families are also still parked.
