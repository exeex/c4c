Status: Active
Source Idea Path: ideas/open/33_riscv_prepared_edge_publication_remaining_stack_destination_sources.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Remaining Stack-Destination Candidates

# Current Packet

## Just Finished

Completed Step 1 inventory for `ideas/open/33_riscv_prepared_edge_publication_remaining_stack_destination_sources.md`.

Current supported `StackSlot` destination policy:

- Shared authority is `prepare::find_unique_indexed_prepared_edge_publication` in `src/backend/mir/riscv/codegen/emit.cpp`, followed by checks that the publication is `Available`, has a matching `Move`, has source and destination homes, and uses `PreparedMoveResolutionOpKind::Move`.
- Source classification is target-local in `render_edge_publication_source_operand`: register homes record `source_register`; rematerializable immediates record `source_immediate_i32`; stack sources record `source_stack_*` only when offset exists and size is 4 or 8; pointer-base sources require a base value name, byte delta, resolvable register base, and record `source_pointer_*`.
- `Register -> StackSlot` I32 is enforced by the stack-destination branch requiring destination kind `StackSlot`, destination offset present, destination size exactly 4, and signed-12-bit destination offset. `has_direct_register_source_for_stack_destination` then requires only `source_register` to be populated before emitting `sw <src>, <offset>(sp)`.
- `RematerializableImmediate -> StackSlot` I32 is enforced by the same destination gate plus `has_rematerializable_i32_source_for_stack_destination`, which requires only `source_immediate_i32` to be populated before emitting `li t0, <imm>` then `sw t0, <offset>(sp)`. The local scratch contract is currently `t0` for this one sequence only; `t5`/`t6` are explicitly reserved for a future address/large-offset helper.
- Current fail-closed stack-destination checks cover missing destination offset, non-I32 destination size, large destination offsets, `StackSlot` sources, `PointerBasePlusOffset` sources, malformed destination homes, missing shared lookups, missing shared publications, and non-move publications.

Remaining candidate matrix:

| Candidate | Existing source facts | Missing target-local policy | Focused positive surface | Focused negative surface | Step 1 selection |
| --- | --- | --- | --- | --- | --- |
| `StackSlot -> StackSlot` | Source helper already records `slot_id`, offset, and size for source stack homes with size 4 or 8; register-destination path already emits `lw`/`ld` with direct or large source offsets. | Needs a scratch-backed stack-source load plus destination store sequence for stack destinations. Define source width acceptance, likely start with I32 only to match destination store; decide scratch register ownership for source value; define same-slot/overlap alias behavior; keep destination offset signed-12-bit unless a separate address helper is added; decide whether source large-offset loads may use existing `t6` address materialization before a `sw` destination store. | Extend `backend_riscv_prepared_edge_publication_test.cpp` around `check_stack_destination_fail_closed_forms` with a shared-publication positive for I32 `StackSlot -> StackSlot`, expected load into scratch then `sw` to destination, and intent fields preserving source and destination stack provenance. | Keep missing-publication clearing negative; retain non-I32 destination rejection; add/keep fail-closed coverage for I64 source to I32 destination, missing source offset/size, aggregate/subword source widths, large source offset if not accepted in the first policy, large destination offset, same/overlapping source-destination slot policy, and non-move publication rejection. | Safest next candidate. It reuses existing stack-source load policy and the existing I32 stack-destination store policy; the only new policy is the scratch/alias sequencing between them. |
| `PointerBasePlusOffset -> StackSlot` | Source helper already resolves pointer base to a register, records base value id/register and byte delta, and register-destination path emits `mv`, `addi`, or large-delta `li` plus `add` when destination does not alias the base. | Needs pointer materialization into a scratch register before store, including zero/small/large delta rules; must define base-register aliasing against scratch and destination store sequence; must decide whether large deltas can use `t0` or require separate address scratch; must keep non-register bases fail-closed; must explicitly separate pointer value materialization from memory load/store alias assumptions. | Later positive for `PointerBasePlusOffset -> StackSlot` should prove zero, small, and large delta materialization to scratch followed by `sw`, all through shared publication lookup. | Existing negatives cover missing base name, missing delta, unresolved base, non-register base, large-delta alias for register destinations, and stack destination unsupported. Add stack-destination-specific negatives for missing shared publication, malformed destination, large destination offset, non-I32 destination, and scratch/base alias policy. | Blocked behind a larger scratch/materialization policy. It is not the safest immediate next slice because it combines pointer materialization, scratch allocation, large-delta policy, and stack store policy. |

## Suggested Next

Implement Step 2 for the narrow `StackSlot -> StackSlot` I32 candidate: add target-local RISC-V policy that loads the source I32 stack slot into an explicit scratch register and stores that scratch to the destination I32 stack slot, only through shared `edge_publications` lookup authority.

## Watchouts

- Keep the first implementation policy intentionally I32 unless the supervisor explicitly widens it; current stack destinations reject non-I32 and this plan does not need to solve I64 stores.
- Decide and document the scratch register before changing code. Reusing `t0` is plausible because immediate-to-stack already owns `t0` for one stack-destination sequence, but stack-source large-offset loading currently uses `t6`, so accepting large source offsets would need an explicit two-scratch/address policy.
- Same source and destination stack slot can be a semantic no-op, but the existing prepared publication path does not currently classify it as a stack-destination optimization. Treat same-slot and overlapping-slot behavior as explicit policy, not an accidental consequence.
- Do not broaden `PointerBasePlusOffset -> StackSlot` in the same slice; that should remain fail-closed until pointer materialization scratch and aliasing policy are separately defined.
- Test surfaces found: `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` for target-local positives/negatives; `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for shared publication lookup authority and source-home preservation.

## Proof

No build or test run by packet contract. Inventory-only proof was source inspection of `src/backend/mir/riscv/codegen/emit.cpp`, `src/backend/mir/riscv/codegen/emit.hpp`, `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`, `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and `tests/backend/bir/CMakeLists.txt`. No `test_before.log` or `test_after.log` was created or modified.
