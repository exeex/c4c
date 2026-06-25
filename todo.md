Status: Active
Source Idea Path: ideas/open/348_vrm_regalloc_mir_and_rv64_assembler_bridge.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Final VRM Allocation And MIR Carrier

# Current Packet

## Just Finished

Completed Step 2: Final VRM Allocation And MIR Carrier. Regalloc now copies
the chosen candidate span placement into assigned vector homes at allocation
time, so MIR-facing homes expose base register, occupied group, contiguous
width, and structured pool/slot identity without waiting for later recovery.
Focused coverage now proves RV64 VR/VRM2/VRM4/VRM8 candidate legality,
non-overlap across independent inline-asm vector groups, and an aligned VRM8
base/group placement.

## Suggested Next

Delegate Step 3: define target-neutral structured grouped vector spill/reload
pseudo records and tests without rendering target assembly at the prepared
layer.

## Watchouts

- Do not add any VRM function-call ABI behavior.
- Do not downgrade supported-path expectations to unsupported.
- The tied/register-matching reuse proof remains covered by existing RV64
  vector inline-asm carrier tests; this packet strengthened final allocation
  base/group identity and VRM8 alignment coverage.
- The delegated owned path `tests/backend/bir/backend_prealloc_inline_asm_test.cpp`
  does not exist in this checkout; the `backend_prealloc_inline_asm` CTest entry
  currently reuses `backend_prepared_printer_test`.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepared_printer|backend_prealloc_inline_asm)$'`

Proof log: `test_after.log`.
