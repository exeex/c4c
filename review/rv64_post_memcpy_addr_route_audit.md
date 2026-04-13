# RV64 Post-Memcpy-Address Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `0faab3e2`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending post-memcpy route selection`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle reset for the current active RV64 lane,
  and `HEAD` still matches that route-selection checkpoint with no newer
  execution commits on top
- commits since base: `0`

## Validation Observed

- no new execution diff since the blocking lifecycle checkpoint, so this audit
  reviewed the live remaining translated `memory.cpp` surface and the current
  shared-header/test reachability instead of new proof logs

## Findings

- Medium: canonical lifecycle state is intentionally blocked and still needs an
  explicit next packet before more execution. The current source idea and
  active runbook both say not to guess among the remaining post-memcpy routes,
  so execution should not continue until `plan.md` and `todo.md` name the next
  bounded seam directly.
- Medium: the remaining translated default load/store owner bodies are still
  too wide for the next packet. The parked Rust mirror at
  `src/backend/riscv/codegen/memory.cpp:201`-`279` still depends on broader
  unresolved shared surface such as `resolve_slot_addr`, `store_for_type`,
  `load_for_type`, `store_t0_to`, and shared F128/default delegation, while the
  current RV64 header export surface at
  `src/backend/riscv/codegen/riscv_codegen.hpp:152`-`168` only reaches the
  already-landed helper seam that the focused tests cover at
  `tests/backend/backend_shared_util_tests.cpp:273`-`311`,
  `514`-`638`.
- Low: the smallest remaining translated seam inside `memory.cpp` is the
  bounded memcpy loop helper at `src/backend/riscv/codegen/memory.cpp:403`-`417`.
  It composes directly with the already-landed memcpy address-loader helpers at
  `src/backend/riscv/codegen/memory.cpp:161`-`189` and advances real owner code
  without first widening into the broader address-resolution and typed
  load/store owner route.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `watch`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Keep the lane in `src/backend/riscv/codegen/memory.cpp`, but narrow the next
packet explicitly to:

- `emit_memcpy_impl_impl(...)`

Allow only the minimum paired shared-surface touches and focused shared-util
coverage that this helper strictly requires.

Keep these families parked for now:

- default load/store delegation
- typed indirect memory-owner bodies
- broader GEP owner bodies
- broader shared RV64 state expansion beyond what the bounded memcpy helper
  strictly needs
- call ABI work

## Rationale

The lane has not drifted; it is blocked because the next packet was
intentionally left unnamed. Among the remaining translated `memory.cpp`
families, the memcpy loop is the smallest still-real owner seam after the
paired address loaders. The broader load/store owner bodies would force missing
slot-resolution and type-selection surfaces first, while the memcpy loop can be
named as a bounded follow-on without reopening the whole memory-owner route.
