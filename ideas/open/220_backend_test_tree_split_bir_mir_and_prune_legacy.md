# Backend Test Tree Split Bir Mir And Prune Legacy

Status: Open
Created: 2026-05-14

Depends On:
- `ideas/open/219_aarch64_natural_operator_naming_and_printer_spelling.md`
- `ideas/open/220_aarch64_markdown_driven_backend_case_bringup.md`

## Goal

Restructure `tests/backend` so backend tests describe the compiler layer they
actually prove:

```text
tests/backend/bir/  -> BIR, prepared BIR, and semantic backend-route tests
tests/backend/mir/  -> target MIR, machine-node, printer, and markdown-driven
                       AArch64 bring-up tests
```

Tests that only survived from an older failed backend route should be deleted
instead of kept as disabled, ambiguous, or misleading backend coverage.

## Why This Idea Exists

The current `tests/backend` directory mixes several eras:

- BIR and prepared-BIR contract tests.
- AArch64 MIR record/model/printer tests.
- Public CLI dump tests for BIR and prepared BIR.
- A small number of real AArch64 `.c -> .s` smoke tests.
- x86/riscv semantic route tests that currently observe BIR behavior.
- Disabled MIR dump/trace shell tests from an earlier route.
- A broad `tests/backend/case` source corpus whose files are not all live for
  the same backend layer.

That makes it too easy to mistake old fixture inventory for active AArch64 MIR
bring-up coverage. It also makes the next markdown-driven AArch64 work harder
to audit because BIR route tests and MIR route tests share one flat namespace.

## In Scope

- Create a clear test tree ownership model:
  - `tests/backend/bir` owns BIR module/model tests, prepared-BIR tests, BIR
    printer/dump tests, and semantic route tests whose assertion target is BIR.
  - `tests/backend/mir` owns target MIR record/model tests, AArch64 machine
    nodes, machine printer tests, target-specific smoke tests, and future
    markdown-driven `.md -> .cpp/.hpp -> case` bring-up tests.
  - Shared `.c` fixtures may remain under a common fixture directory only when
    they are explicitly treated as input data, not as proof ownership.
- Move or rewrite CMake wiring so test names, labels, source paths, and helper
  functions match the BIR/MIR ownership split.
- Add or update inventory documentation that classifies current backend tests
  as:
  - `bir-live`
  - `mir-live`
  - `shared-fixture`
  - `legacy`
  - `deprecated`
  - `delete`
- Delete invalid tests whose only purpose is preserving a past failed route.
- Remove disabled MIR shell placeholders unless they are converted into an
  active MIR contract under `tests/backend/mir`.
- Keep the AArch64 external `.s` smoke route from idea 216 as MIR-owned proof.
- Keep idea 220's markdown-driven rule: open MIR case coverage only when the
  owning AArch64 markdown feature has a structured implementation route.

## Out Of Scope

- Implementing new AArch64 backend capability just to justify moving a test.
- Weakening expectations, marking supported cases unsupported, or preserving
  narrow fixture checks as fake progress.
- Treating external `.s` or `.ll` as backend input.
- Replacing the terminal `.s` printer or implementing the future internal
  assembler/object/linker stack.
- Flattening x86/riscv route tests into MIR unless they actually prove target
  MIR behavior.

## Initial Classification Notes

Current live AArch64 MIR case coverage is only:

- `tests/backend/case/aarch64_no_selected_machine_nodes.c`
- `tests/backend/case/aarch64_return_zero_smoke.c`

Current live AArch64 unit/model tests named `backend_aarch64_*` are MIR-owned.

Current `backend_prepare_*`, `backend_prepared_printer`, `backend_lir_to_bir_*`,
and `backend_cli_dump_bir*` / `backend_cli_dump_prepared_bir*` tests are
BIR-owned unless an audit proves their assertion target is later than prepared
BIR.

Current `backend_codegen_route_x86_64_*_observe_semantic_bir` and riscv64
semantic route tests are BIR-owned while their expected snippets observe BIR or
prepared-BIR semantics. They are not AArch64 MIR bring-up proof.

Current disabled `backend_cli_dump_mir_*` and `backend_cli_trace_mir_*` tests
are suspected legacy/deprecated placeholders. They should either be replaced by
real MIR-owned tests or deleted.

## Acceptance Criteria

- `tests/backend/bir` exists and owns the current BIR/prepared-BIR backend
  tests.
- `tests/backend/mir` exists and owns the current AArch64 MIR tests.
- CMake wiring references the new paths and labels tests so `bir` and `mir`
  subsets can be selected independently.
- The backend case corpus is classified so fixture files are not mistaken for
  enabled MIR coverage.
- Disabled or misleading legacy tests are removed, not kept indefinitely as
  stale route memory.
- Existing live behavior is preserved after moving tests; any deleted test has
  a recorded reason in the inventory or commit message.
- The restructuring does not enable new AArch64 case coverage ahead of the
  markdown owner required by idea 220.

## Reviewer Reject Signals

- A test is moved under `mir` while still only proving BIR text output.
- A disabled legacy test remains without a concrete owner and next action.
- A fixture file is treated as active coverage just because it exists in the
  source tree.
- CMake labels cannot run `bir` and `mir` subsets separately.
- Test deletion hides a currently live contract instead of removing stale route
  debris.
- The cleanup becomes a testcase-overfit rewrite or expands AArch64 codegen
  functionality without a markdown owner.
