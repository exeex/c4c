Status: Active
Source Idea Path: ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Expose proof through prepared printer and diagnostics

# Current Packet

## Just Finished

Step 4, "Expose proof through prepared printer and diagnostics", completed for
CRC/vector prepared carrier proof visibility.

- Verified the prepared printer already exposes structured intrinsic carrier
  facts rather than selected machine instruction output.
- Added explicit printer assertions for complete `Crc/Crc32W`,
  `VectorMemory/VectorLoad`, and `VectorOperation/VectorAdd` carriers.
- Kept diagnostic proof for incomplete carriers: CRC stays missing without
  prepared operand homes, and malformed vector-load shape reports the structured
  missing fact instead of printing a complete carrier.

## Suggested Next

Start Step 5 by adding or adjusting AArch64 boundary tests proving CRC/vector
prepared carriers do not yet select or print AArch64 machine records under this
idea.

## Watchouts

- The prepared-printer proof intentionally stops at carrier facts and missing
  fact diagnostics; it does not assert AArch64 machine instruction spelling,
  MIR selection, or final assembly as authority.
- The CRC fail-closed path still uses an immediate accumulator to prove a
  populated call plan cannot complete without prepared operand-home authority.
- Vector values still use the existing `I128` storage lane, with vector
  semantic authority carried by explicit vector fields.

## Proof

Proof command:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_printer'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
