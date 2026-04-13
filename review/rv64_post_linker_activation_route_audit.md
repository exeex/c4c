# RV64 Post-Linker-Activation Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `aff3e802`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 binary-utils lane to linker activation`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `aff3e802` as the latest canonical
  lifecycle checkpoint, and that rewrite is the one that activated the current
  Step 2 linker route now described in
  [plan.md](/workspaces/c4c/plan.md:144) and
  [todo.md](/workspaces/c4c/todo.md:36)
- commits since base: `1`
  (`54a7b875 rv64: activate first static linker seam for bounded slice`)

## Scope Reviewed

- `plan.md`
- `todo.md`
- `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`
- `src/backend/riscv/linker/mod.hpp`
- `src/backend/riscv/linker/mod.cpp`
- `src/backend/riscv/linker/link.cpp`
- `src/backend/riscv/linker/emit_exec.cpp`
- `CMakeLists.txt`
- `tests/backend/backend_shared_util_tests.cpp`
- `rv64_linker_activation_build.log`
- `rv64_linker_activation_contract_test.log`
- `rv64_linker_activation_static_test.log`

## Findings

- Medium: commit `54a7b875` truthfully lands the first bounded linker-activation
  packet selected by `aff3e802`. The tree now exposes a named RV64 linker
  contract surface through the new public header, wires the minimum linker
  files into the active test build, and replaces the prior `link.cpp`
  translation-note gap with a real first-static path:
  [src/backend/riscv/linker/mod.hpp](/workspaces/c4c/src/backend/riscv/linker/mod.hpp:14),
  [src/backend/riscv/linker/mod.hpp](/workspaces/c4c/src/backend/riscv/linker/mod.hpp:60),
  [CMakeLists.txt](/workspaces/c4c/CMakeLists.txt:489),
  [src/backend/riscv/linker/link.cpp](/workspaces/c4c/src/backend/riscv/linker/link.cpp:82),
  [src/backend/riscv/linker/link.cpp](/workspaces/c4c/src/backend/riscv/linker/link.cpp:212),
  [src/backend/riscv/linker/link.cpp](/workspaces/c4c/src/backend/riscv/linker/link.cpp:299).
- Medium: the landed seam stays inside the packet bounds the active plan asked
  for, rather than widening into broader linker/runtime work. `link_builtin(...)`
  explicitly rejects library, CRT, and unsupported linker-arg inputs, and
  `link_shared(...)` remains an explicit non-activated stub, so the new surface
  is truthful about being only the first bounded static-exec seam for the
  current object lane:
  [src/backend/riscv/linker/link.cpp](/workspaces/c4c/src/backend/riscv/linker/link.cpp:312),
  [src/backend/riscv/linker/link.cpp](/workspaces/c4c/src/backend/riscv/linker/link.cpp:324),
  [src/backend/riscv/linker/link.cpp](/workspaces/c4c/src/backend/riscv/linker/link.cpp:338),
  [plan.md](/workspaces/c4c/plan.md:167).
- Low: focused proof matches the route, but the two committed test logs are
  zero-byte artifacts again. That is consistent with this harness behavior,
  because direct reviewer reruns of
  `test_riscv_linker_contract_exposes_first_static_surface` and
  `test_riscv_first_static_executable_image_matches_return_add_object` both
  exited `0` with no output. The new tests do prove the intended seam: header
  exposure plus the current return-add object flowing into a minimal ET_EXEC
  RV64 image:
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9173),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9188),
  [rv64_linker_activation_build.log](/workspaces/c4c/rv64_linker_activation_build.log),
  [rv64_linker_activation_contract_test.log](/workspaces/c4c/rv64_linker_activation_contract_test.log),
  [rv64_linker_activation_static_test.log](/workspaces/c4c/rv64_linker_activation_static_test.log).
- Low: the packet intentionally stops short of a broader linker route. Text
  relocations are only accepted when absent, and the emitted image is the
  minimal single-segment executable needed for the current relocation-free
  return-add object lane, which matches the active plan but means the next
  executor packet must be chosen by lifecycle rewrite rather than inferred
  locally:
  [src/backend/riscv/linker/emit_exec.cpp](/workspaces/c4c/src/backend/riscv/linker/emit_exec.cpp:448),
  [src/backend/riscv/linker/emit_exec.cpp](/workspaces/c4c/src/backend/riscv/linker/emit_exec.cpp:479),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9224),
  [plan.md](/workspaces/c4c/plan.md:182).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `watch`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Commit `54a7b875` did truthfully land the first bounded RV64 linker-activation
packet selected by the current lifecycle state.

Execution should not continue with another executor packet until canonical
lifecycle state is rewritten. This is not because the route drifted; it is
because Step 2 now appears complete and
[plan.md](/workspaces/c4c/plan.md:182) already makes the next action a fresh
re-audit before selecting any follow-on linker packet. The broader RV64 linker
route still looks valid, but `plan.md` and `todo.md` should be updated from
this new checkpoint before more execution.

## Validation Notes

- `rv64_linker_activation_build.log` shows the focused rebuild of
  `backend_shared_util_tests`
- `rv64_linker_activation_contract_test.log` is empty
- `rv64_linker_activation_static_test.log` is empty
- reviewer reran:
  - `./build/backend_shared_util_tests test_riscv_linker_contract_exposes_first_static_surface`
  - `./build/backend_shared_util_tests test_riscv_first_static_executable_image_matches_return_add_object`
- both reruns exited `0` and produced no output, matching the test harness
  behavior for passing filtered runs
