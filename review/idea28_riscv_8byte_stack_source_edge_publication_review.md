# Idea 28 RISC-V 8-Byte Stack Source Edge Publication Review

Active source idea path: `ideas/open/28_riscv_prepared_edge_publication_stack_source_policy_broadening.md`

Review base: `e84ccb337` (`[plan] Activate RISC-V stack source policy broadening plan`)

Base rationale: this is the active-idea lifecycle activation checkpoint for idea 28. The only later lifecycle commit before the implementation, `8f24b8352`, records Step 1 inventory in `todo.md`; it does not reset, rewrite, or reviewer-checkpoint the source idea or plan. Reviewing `e84ccb337..HEAD` covers the full active idea slice.

Commits since base: 2

Reviewed focus:

- `58e9f108e` (`Support RISC-V 8-byte stack source edge publications`)
- current `todo.md` metadata repair advancing to Step 3 review state
- RISC-V helper and focused prepared edge-publication tests

## Findings

No blocking findings.

## Evidence

- Source idea asks for one stack-source form beyond the idea 25 4-byte concrete-offset route, through shared `edge_publications`, with unsupported forms explicit and fail-closed (`ideas/open/28_riscv_prepared_edge_publication_stack_source_policy_broadening.md:43`).
- Plan Step 2 scopes implementation to the selected stack-source form, requires shared lookup authority, preserves existing 4-byte behavior, and keeps unsupported widths, missing offsets, dynamic addressing, large offsets, malformed facts, and non-move publications fail-closed (`plan.md:97`).
- The helper accepts `StackSlot -> Register` sources only when `offset_bytes` exists and `size_bytes` is 4 or 8; the newly broadened 8-byte form is additionally guarded by the signed-12-bit load offset predicate (`src/backend/mir/riscv/codegen/emit.cpp:79`).
- Instruction rendering selects `ld` only for recorded 8-byte stack-source intents and otherwise preserves the existing `lw` path for stack loads (`src/backend/mir/riscv/codegen/emit.cpp:176`).
- Positive coverage mutates the prepared value home to an 8-byte stack slot, verifies emitted `ld a1, 32(sp)`, verifies the helper intent is available through the shared publication, and then clears `publications_by_edge_destination` to prove the helper does not rediscover the edge move locally (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:273`).
- Negative coverage proves missing offset, subword width, aggregate width, 8-byte offset overflow, and non-move publication cases fail closed (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:330`).
- Existing unsupported-width coverage was adjusted from 8 bytes to 16 bytes because 8 bytes is now intentionally supported; this is not an expectation downgrade (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:771`).
- `main()` runs the new positive and fail-closed checks alongside the pre-existing register, immediate, 4-byte stack-source, pointer-base, and stack-destination checks (`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:842`).
- Current `todo.md` metadata is aligned with Step 3 review state and records focused proof, matching regression guard, and broader backend validation (`todo.md:4`, `todo.md:52`).

## Judgments

Idea alignment: matches source idea.

Runbook transcription: plan matches idea.

Route alignment: on track.

Technical debt: acceptable.

Validation sufficiency: narrow proof plus backend bucket sufficient for handoff/closure decision.

Reviewer recommendation: continue current route.

## Notes

The current slice deliberately leaves sub-word, unsigned/floating 32-bit, dynamic-address, aggregate, and large-offset stack-source forms fail-closed. That matches the idea's requirement to keep unsupported stack-source forms explicit instead of silently claiming broad support.
