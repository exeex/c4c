# Idea 27 RISC-V Stack Destination Edge Publication Review

Active source idea path: `ideas/open/27_riscv_prepared_edge_publication_stack_destination_support.md`

Chosen base commit: `12d310f57 [plan] Activate RISC-V stack destination edge publication plan`

Base rationale: this commit created the active idea 27 runbook and aligned `todo.md` with the source idea. The later `5e44fd821` commit is an inventory-only execution note, not a route-reset or reviewer checkpoint, so the activation commit is the right active-idea checkpoint.

Commit count since base: 2

Reviewed range: `12d310f57..HEAD`, plus the current uncommitted `todo.md` metadata repair that advances the packet from Step 3 to Step 4.

## Findings

No blocking findings.

## Alignment Notes

- The implementation matches the source idea. The new RISC-V consumer still enters through `find_unique_indexed_prepared_edge_publication` and only emits after shared `edge_publications` authority is present (`src/backend/mir/riscv/codegen/emit.cpp:123`, `src/backend/mir/riscv/codegen/emit.cpp:188`).
- The supported rule is the intended narrow semantic slice: `Register -> StackSlot` with concrete destination offset and 4-byte size, emitted as `sw <src>, <offset>(sp)` in the RISC-V backend (`src/backend/mir/riscv/codegen/emit.cpp:188`).
- Existing register-destination behavior is preserved in the same helper before the stack-destination branch (`src/backend/mir/riscv/codegen/emit.cpp:162`).
- The positive test proves emitted stack store text, preserved shared publication pointer, source/destination ids, destination stack facts, and target-local `sw` rendering (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:344`).
- The authority-removal test clears the shared edge-publication index and expects `MissingPublication`, so this is not local predecessor/successor rediscovery (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:390`).
- Negative coverage keeps malformed stack destinations, immediate sources, stack sources, pointer-base sources, and non-move publications fail-closed (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:400`).
- The current dirty `todo.md` change only repairs execution metadata from Step 3 to Step 4. It does not change route intent.

## Judgments

- Idea alignment: `matches source idea`
- Runbook transcription: `plan matches idea`
- Route alignment: `on track`
- Technical debt: `acceptable`
- Validation sufficiency: `narrow proof sufficient`

Validation basis: `todo.md` records the delegated focused proof command as passing 5/5, the matching focused regression guard as passing 5/5 before and after, and broader backend validation as passing 163/163. Current `test_before.log` contains the focused 5/5 evidence, and current `test_after.log` contains the broader backend 163/163 evidence.

## Recommendation

`continue current route`

Proceed to Step 5 handoff/closure. The handoff should scope support exactly to focused RISC-V `Register -> StackSlot` stack-destination edge publications with concrete 4-byte stack destination facts, and preserve caveats that immediate, stack-source, pointer-base, malformed stack destinations, and other source-to-stack forms remain unsupported/fail-closed.
