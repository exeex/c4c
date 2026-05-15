Status: Active
Source Idea Path: ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extend prepared intrinsic carriers

# Current Packet

## Just Finished

Step 3, "Extend prepared intrinsic carriers", completed for CRC/vector
prepared carrier facts.

- Extended prepared intrinsic carriers to preserve required feature, operand
  roles, all operands, vector shape, signedness, memory/access facts,
  immediate flags, and operand/result home authority from BIR intrinsic facts.
- Added family-specific completeness checks for scalar FAbs, `Crc/Crc32W`,
  `VectorMemory/VectorLoad`, and `VectorOperation/VectorAdd`.
- Kept CRC/vector completion fail-closed when semantic facts or prepared
  register/home authority are missing; ordinary call-plan presence alone is
  not sufficient.
- Extended prepared-printer output and tests for accepted CRC/vector carriers,
  malformed vector facts, and missing operand-home authority.

## Suggested Next

Start Step 4 by verifying the prepared-printer and diagnostic proof surface
against `plan.md`: the current slice may already satisfy most of Step 4, so the
supervisor should check remaining prepared-printer/diagnostic gaps before
moving to Step 5.

## Watchouts

- The prepared carrier records vector values on the existing `I128` storage
  lane and preserves vector semantic authority through explicit vector fields.
- The current fail-closed home-authority test uses an immediate CRC operand to
  prove a populated call plan cannot complete without a corresponding prepared
  operand home.
- `PreparedIntrinsicCarrier` now stores full `PreparedValueHome` snapshots for
  operands/results; downstream consumers should use them as prepared-route
  authority instead of re-resolving raw names.

## Proof

Proof command:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prepared_printer|lir_to_bir_notes)'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
