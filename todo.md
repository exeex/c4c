Status: Active
Source Idea Path: ideas/open/31_riscv_prepared_edge_publication_stack_source_policy_followup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Remaining Stack-Source Candidates

# Current Packet

## Just Finished

Completed Step 1 - Inventory Remaining Stack-Source Candidates.

Current concrete-offset support is enforced in
`src/backend/mir/riscv/codegen/emit.cpp`:

- Shared authority: `consume_edge_publication_move_intent` reaches emission only
  after `find_unique_indexed_prepared_edge_publication` returns an available
  publication whose prepared move is a `Move`.
- Stack-source eligibility: `render_edge_publication_source_operand` accepts
  only `PreparedValueHomeKind::StackSlot` with `offset_bytes`, `size_bytes` of
  4 or 8, and for 8-byte sources an offset accepted by
  `fits_signed_12_bit_load_offset`.
- Emission: register destinations render stack sources as `lw <dst>,
  <offset>(sp)` for size 4 and `ld <dst>, <offset>(sp)` for size 8.
- Existing focused tests are in
  `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`:
  `check_stack_slot_to_register_move_uses_shared_lookup` covers 4-byte `lw`,
  `check_stack_slot_i64_to_register_move_uses_shared_lookup` covers 8-byte
  `ld`, and shared-authority removal checks verify RISC-V does not rediscover
  edge moves without prepared lookup facts.
- Current fail-closed coverage rejects missing offset, missing size,
  unsupported subword width, aggregate width, 8-byte offset 2048, and non-move
  publications. The large-offset rejection is currently specific to 8-byte
  sources; 4-byte stack sources are accepted regardless of immediate range, so
  the next implementation must preserve or correct that deliberately rather
  than silently relying on malformed assembly.

Remaining candidate policy matrix:

| Candidate | Current status | Missing target-local policy |
| --- | --- | --- |
| Sub-word integer stack source | Fail-closed for size 2; size 1/2 not accepted. | Decide signedness authority before choosing `lb`/`lh` versus `lbu`/`lhu`; define how the loaded value is extended into the destination GPR; add offset-range enforcement and focused positive/negative coverage. |
| Unsigned 32-bit stack source | Not distinguishable from existing size-4 `lw`; prepared value homes carry size but no signedness/type. | Need semantic signedness or value type authority before choosing `lwu`; otherwise accepting this would be indistinguishable from current signed/ABI-extended `lw` behavior. |
| Floating 32-bit stack source | Not distinguishable from existing size-4 stack source by home alone; current destination path treats all register destinations as text register names and emits `lw`. | Need register-bank/type authority for FPR destinations and an explicit `flw` policy; also need tests proving FP stack-source publication consumes shared edge facts and rejects GPR-only/source-type ambiguity. |
| Large-offset stack loads | 8-byte size 2048 is fail-closed; 4-byte large offset currently slips through the size-4 branch. | Define scratch/address materialization contract, likely `li` plus `add` into a scratch such as `t5`/`t6`, then load from `0(scratch)`; fix offset-range handling consistently for 4-byte and 8-byte forms. |
| Dynamic-address stack sources | No representation in `StackSlot` homes; pointer-like forms are `PointerBasePlusOffset` and are out of scope for this plan. | Concrete dynamic stack-source address representation and scratch contract are missing; this is blocked unless a prepared home or address materialization fact identifies the dynamic base. |
| Aggregate-width stack sources | Fail-closed for size 16. | Need multi-register or memory-copy destination policy, register-pair layout, scratch ownership, and explicit size/alignment constraints; not safe as a first follow-up. |

Safest next candidate: fix and implement large-offset concrete `StackSlot ->
Register` loads, because it preserves the existing size-4/size-8 scalar load
model and only adds target-local address materialization policy. The packet
should first make the offset-range gate explicit for both 4-byte `lw` and
8-byte `ld`, then add a scratch-backed large-offset path with tests for size-4
and size-8 concrete stack sources plus a negative alias/scratch or unsupported
neighbor case.

## Suggested Next

Proceed to Step 2 by implementing the large-offset concrete stack-source load
policy for `StackSlot -> Register`, including an explicit 12-bit immediate
gate for existing direct `lw`/`ld` forms and a target-local scratch-backed
materialization path for offsets outside that range.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority for edge moves.
- Do not broaden pointer-base register-destination or source-to-stack destination policy in this plan.
- Preserve existing concrete-offset 4-byte `lw` and 8-byte `ld` support.
- Treat fixture-name matching, expectation-only changes, and predecessor/successor block scans as route failures.
- `PreparedValueHome` currently records stack offset/size but not signedness,
  scalar type, or register bank, so sub-word, unsigned 32-bit, and F32 policies
  need additional authority before implementation.
- Direct 4-byte stack-source support currently lacks the 12-bit load-immediate
  guard that 8-byte support has; Step 2 should make that behavior intentional.

## Proof

No proof was run because this was an inventory-only packet. `test_after.log`
was not modified.
