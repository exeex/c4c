Status: Active
Source Idea Path: ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Guard future AArch64 selection boundaries

# Current Packet

## Just Finished

Step 5, "Guard future AArch64 selection boundaries", completed for CRC/vector
prepared carrier selection and printer boundaries.

- Added AArch64 dispatch fixtures with complete `Crc/Crc32W`,
  `VectorMemory/VectorLoad`, and `VectorOperation/VectorAdd` prepared intrinsic
  carrier authority.
- Asserted those complete CRC/vector carriers stay fail-closed at the MIR
  boundary: diagnostics report an unsupported intrinsic family, only the return
  terminator is emitted, and no intrinsic or call machine node is selected.
- Added printer-boundary assertions that fabricated complete CRC/vector
  intrinsic-shaped records do not print AArch64 machine instruction lines.
- Preserved the existing scalar `FAbs` selected-machine behavior and existing
  incomplete/unsupported path coverage in the delegated subset.

## Suggested Next

Run Step 6 from `plan.md`: acceptance validation for the carrier dependency
route, including a fresh build, the narrow BIR/prepared/AArch64 boundary tests,
and broader backend validation as appropriate before the commit boundary.

## Watchouts

- CRC/vector prepared carriers now have positive proof through BIR/prepared
  carrier layers and explicit negative proof at the AArch64 machine boundary.
- The Step 5 tests intentionally do not add CRC/NEON selected-machine support or
  final assembly spelling.

## Proof

Proof command:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_instruction_dispatch|aarch64_machine_printer|prepared_printer|lir_to_bir_notes)'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
