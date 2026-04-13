# RV64 Post-GEP-Const Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `341cb527`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 lane
  past pointer resolution helper seam`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle reset for the current active RV64 lane,
  and the only implementation commit since then is the newly landed GEP
  constant-helper slice
- commits since base: `1`

## Validation Observed

- `cmake --build build --target backend_shared_util_tests c4cll`
  - log: `rv64_gep_const_build.log`
- `./build/backend_shared_util_tests riscv_translated_memory_helper`
  - log: `rv64_gep_const_shared_util.log`
- `./build/backend_shared_util_tests riscv_codegen_header_exports_translated_memory_helper`
  - log: `rv64_gep_const_header_shared_util.log`

All three commands exited successfully.

## Findings

- Medium: canonical lifecycle state is now stale because `plan.md` and
  `todo.md` still point at the just-finished GEP constant-helper packet.
  Before more execution, rewrite the active packet so the lane does not drift
  into whichever remaining `memory.cpp` family happens to look convenient.
- Low: the new implementation itself stays aligned with the active route.
  `src/backend/riscv/codegen/memory.cpp:144`-`159` lands only
  `emit_gep_direct_const_impl(...)` and `emit_gep_indirect_const_impl(...)`,
  while the matching shared-header reachability and focused text checks live at
  `src/backend/riscv/codegen/riscv_codegen.hpp:165`-`166` and
  `tests/backend/backend_shared_util_tests.cpp:283`-`300`,
  `530`-`579`.
- Low: the remaining nearby work now splits cleanly into a helper-sized next
  seam and several wider parked families. The smallest still-plan-aligned
  follow-on inside `memory.cpp` is the paired memcpy address-loader helpers at
  `src/backend/riscv/codegen/memory.cpp:351`-`370`:
  `emit_memcpy_load_dest_addr_impl(...)` and
  `emit_memcpy_load_src_addr_impl(...)`.
  They reuse the already-landed alloca-alignment, assigned-register, and
  s0-relative load patterns without forcing the broader default load/store
  owner bodies at `171`-`247` or the full byte-copy loop at `373`-`386`.

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `watch`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Keep the lane in `src/backend/riscv/codegen/memory.cpp`, but narrow the next
packet explicitly to:

- `emit_memcpy_load_dest_addr_impl(...)`
- `emit_memcpy_load_src_addr_impl(...)`

Allow only the minimum paired shared-surface touches and focused shared-util
coverage those two helper bodies strictly require.

Keep these families parked for now:

- full default load/store owner bodies
- typed indirect load/store helpers
- full `memcpy` loop emission (`emit_memcpy_impl_impl(...)`)
- broader GEP owner bodies
- broader shared RV64 state expansion
- call ABI work

## Rationale

The current branch has not drifted from the active RV64 integration lane, but
the next step is no longer encoded in canonical lifecycle state. If execution
continues without a rewrite, the remaining `memory.cpp` surface is wide enough
to invite packet-boundary drift. The paired memcpy address-loader helpers are
the smallest remaining translated seam that still advances real owner code
behind the existing RV64 header surface without claiming that the broader
`memcpy` owner or typed indirect memory paths are ready.
