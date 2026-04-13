# RV64 Post-Writer-Activation Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `31263960`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: route rv64 assembler lane to writer activation`)
- why this is the right checkpoint:
  `scripts/plan_change_gap.sh` reports `31263960` as the latest canonical
  lifecycle checkpoint, and that rewrite is the one that activated the current
  writer-backed route after the direct assembler packet was explicitly marked
  complete and exhausted
- commits since base: `1`
  (`57a614e1 rv64: activate writer-backed assembler emission for bounded slice`)

## Scope Reviewed

- `plan.md`
- `todo.md`
- `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`
- `src/backend/riscv/assembler/mod.cpp`
- `src/backend/riscv/assembler/elf_writer.cpp`
- `tests/backend/backend_shared_util_tests.cpp`
- `rv64_writer_activation_build.log`
- `rv64_writer_activation_route_test.log`
- `rv64_writer_activation_variant_test.log`

## Findings

- Medium: commit `57a614e1` truthfully lands the first bounded
  writer-activation packet selected by the current lifecycle state. The active
  RV64 assembler path no longer calls the activation-specific
  `write_minimal_elf_object(...)`; it now routes the parser-backed bounded
  slice through `write_elf_object(...)`, which drives
  `ElfWriter::process_statements(...)` and `ElfWriter::write_elf(...)` before
  object emission:
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:9),
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:24),
  [src/backend/riscv/assembler/mod.cpp](/workspaces/c4c/src/backend/riscv/assembler/mod.cpp:40),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:536),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:569),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:602).
- Medium: the packet stays inside the bounded Step 2 contract instead of
  widening into broader writer/directive work. `process_statements(...)`
  accepts only the already-planned minimal activation statement family, and
  `write_elf(...)` still emits the same bounded object bytes through the writer
  seam by parameterizing the existing minimal ELF builder with `elf_class` and
  `elf_flags`:
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:178),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:226),
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:582).
- Low: the focused proof matches the route, but the two committed test logs are
  zero-byte files. That is not contradictory here because the backend test
  harness prints only on failure, and direct reviewer reruns of
  `test_backend_shared_assemble_target_lir_module_matches_public_wrappers` and
  `test_riscv_builtin_assembler_parses_activation_slice_variants` both exited
  `0` with no output. Still, these artifacts preserve less execution context
  than earlier textual logs:
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9206),
  [tests/backend/backend_test_util.hpp](/workspaces/c4c/tests/backend/backend_test_util.hpp:19),
  [tests/backend/backend_test_util.hpp](/workspaces/c4c/tests/backend/backend_test_util.hpp:24),
  [tests/backend/backend_test_util.hpp](/workspaces/c4c/tests/backend/backend_test_util.hpp:32),
  [tests/backend/backend_test_util.hpp](/workspaces/c4c/tests/backend/backend_test_util.hpp:34).
- Low: Step 2 is complete, but this packet does not imply a same-route follow-on.
  The writer seam is now active for the bounded `.text` / `.globl main` /
  `main:` / `addi` / `ret` slice, while broader directive execution,
  relocation-heavy writing, and wider assembler coverage remain visibly parked.
  `write_minimal_elf_object(...)` also remains as a local helper, so the next
  truthful step still requires a fresh lifecycle decision rather than assuming
  another executor packet on the same route:
  [src/backend/riscv/assembler/elf_writer.cpp](/workspaces/c4c/src/backend/riscv/assembler/elf_writer.cpp:610),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9115),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:9141).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `watch`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Commit `57a614e1` did truthfully land the first bounded writer-backed
activation-slice packet selected by the current lifecycle state.

Execution should not continue on the same route without a lifecycle rewrite.
The active plan explicitly makes Step 3 a re-audit after the first
writer-backed packet, and that condition is now satisfied. Canonical state
should therefore record `57a614e1` as the completed Step 2 packet and choose
the next truthful continuation from the new checkpoint instead of issuing
another executor packet by implication.

## Validation Notes

- `rv64_writer_activation_build.log` shows the focused rebuild of
  `backend_shared_util_tests`
- `rv64_writer_activation_route_test.log` is empty
- `rv64_writer_activation_variant_test.log` is empty
- reviewer reran:
  - `./build/backend_shared_util_tests test_backend_shared_assemble_target_lir_module_matches_public_wrappers`
  - `./build/backend_shared_util_tests test_riscv_builtin_assembler_parses_activation_slice_variants`
- both reruns exited `0` and produced no output, which matches the test
  harness behavior for passing filtered runs
