Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select CRC And Vector Intrinsic Records

# Current Packet

## Just Finished

Completed `plan.md` Step 3 selected dispatch for the ready AArch64 intrinsic
families only.

- Complete `Crc/Crc32W`, `VectorMemory/VectorLoad`, and
  `VectorOperation/VectorAdd` prepared carriers now lower through dispatch into
  the selected machine-node records created by Step 2.
- Dispatch preserves prepared operand/result register authority for CRC W
  registers, vector-load pointer/result registers, and vector-add vector
  lhs/rhs/result registers.
- Vector-load selection synthesizes the structured pointer memory operand from
  carrier authority without adding final printer assembly spelling.
- Existing scalar `FAbs` selected dispatch remains green.
- Unsupported or incomplete intrinsic carriers still fail closed through
  diagnostics instead of becoming ordinary calls.

## Suggested Next

Execute `plan.md` Step 4: print CRC32W, vector-load, and vector-add selected
records from structured selected-record fields. Reject incomplete or
non-selected records, and do not use intrinsic spelling or generic call plans
as printer authority.

## Watchouts

- New CRC/vector machine opcodes deliberately have no printer mnemonic; final
  assembly spelling belongs to a later packet.
- Do not print from intrinsic spelling alone, generic call plans, archived
  scratch registers, or final assembly text.
- Barrier/cache/pause-hint/builtin-address work is blocked on upstream semantic
  and prepared intrinsic carriers; adding name-only records for these families
  would be route drift.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.

## Proof

Passed delegated proof; output preserved at `test_after.log`.

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_instruction_dispatch|aarch64_target_instruction_records|aarch64_machine_printer|prepared_printer|lir_to_bir_notes)'; } 2>&1 | tee test_after.log`

`git diff --check` passed.
