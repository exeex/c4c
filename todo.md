Status: Active
Source Idea Path: ideas/open/241_aarch64_crc_vector_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extend BIR semantic intrinsic facts

# Current Packet

## Just Finished

Step 2, "Extend BIR semantic intrinsic facts", completed for BIR-only
semantic representatives.

- Added BIR intrinsic families and operations for `Crc/Crc32W`,
  `VectorMemory/VectorLoad`, and `VectorOperation/VectorAdd`.
- Extended `bir::IntrinsicOperation` with required feature identity, operand
  roles, vector element/lane/width facts, signedness, memory operand/access
  facts, and immediate-presence fields.
- Lowered accepted AArch64 representatives into structured BIR facts:
  `llvm.aarch64.crc32w`, `llvm.aarch64.neon.ld1.v16i8.p0i8`, and
  `llvm.aarch64.neon.add.v16i8`.
- Added BIR notes tests proving the accepted CRC/vector representatives and
  fail-closed behavior for x86-only, wrong CRC type, missing vector-load
  pointer, and mismatched vector-lane/type cases.
- Prepared carrier completion remains intentionally unwired for Step 3.

## Suggested Next

Start Step 3 by extending prepared intrinsic carriers to consume these BIR
facts, require family-specific completeness, and keep CRC/vector carriers
missing until register/home authority and prepared call-plan facts are present.

## Watchouts

- Vector values are represented on the existing 16-byte `I128` storage lane at
  the BIR call boundary; semantic vector authority is the explicit element,
  lane, and width facts on `IntrinsicOperation`.
- Recognized malformed AArch64 semantic intrinsic calls and x86-only LLVM
  intrinsic spellings are blocked from generic direct-call fallback so they
  fail closed at the BIR notes level.
- Do not wire prepared completion from ordinary call-plan presence alone;
  Step 3 still needs register/home authority before marking carriers complete.

## Proof

Proof command:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_lir_to_bir_notes'; } 2>&1 | tee test_after.log`

Result: passed. Proof log: `test_after.log`.
