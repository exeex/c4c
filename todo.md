Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Selected Intrinsic Machine Records

# Current Packet

## Just Finished

Completed `plan.md` Step 2 selected-record definitions for the ready AArch64
intrinsic families only.

- Added selected machine-node record shapes and direct constructors for
  `Crc/Crc32W`, `VectorMemory/VectorLoad`, and `VectorOperation/VectorAdd`.
- CRC32W records preserve source carrier provenance, family/operation,
  required feature, I32 operand/result types, accumulator/data roles, unsigned
  semantics, operand registers, result register, source callee, and prepared
  call-plan authority.
- Vector-load records preserve source carrier provenance, NEON feature,
  pointer operand role/register, v16i8 shape, read memory access, structured
  pointer-memory facts, I128 result register, source callee, and prepared
  call-plan authority.
- Vector-add records preserve source carrier provenance, NEON feature, vector
  lhs/rhs roles/registers, v16i8 shape, no-memory access, I128 result register,
  source callee, and prepared call-plan authority.
- Record-level tests now prove these fields can be carried by selected machine
  nodes without adding prepared-carrier dispatch selection or printer assembly
  text.

## Suggested Next

Execute `plan.md` Step 3 for the ready families only: wire complete CRC32W,
vector-load, and vector-add prepared intrinsic carriers into the new selected
record constructors, while preserving the existing scalar `FAbs` selected path
and keeping unsupported or incomplete carriers fail-closed.

## Watchouts

- Step 2 intentionally did not edit `dispatch.cpp` or `machine_printer.cpp`.
- New CRC/vector machine opcodes deliberately have no printer mnemonic; final
  assembly spelling belongs to a later packet.
- Treat scalar `FAbs` selected-machine support as existing behavior to preserve.
- Do not select or print from intrinsic spelling alone, generic call plans,
  archived scratch registers, or final assembly text.
- Barrier/cache/pause-hint/builtin-address work is blocked on upstream semantic
  and prepared intrinsic carriers; adding name-only records for these families
  would be route drift.
- Keep unsupported x86-only and incomplete-fact paths fail-closed.

## Proof

Passed delegated proof; output preserved at `test_after.log`.

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_target_instruction_records|aarch64_instruction_dispatch|aarch64_machine_printer)'; } 2>&1 | tee test_after.log`

`git diff --check` passed.
