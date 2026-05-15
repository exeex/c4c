Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Print Supported Binary128 Machine Nodes

# Current Packet

## Just Finished

Step 5 completed the first binary128 printer packet. AArch64 machine printing
now emits structured F128 load/store transport from selected full-width
q-register carriers and emits selected F128 helper-boundary call sequences from
record-level q-register/scalar marshal facts. The printer still rejects
memory-backed F128 transport without a printable q-register and incomplete F128
helper records through diagnostics instead of reconstructing operands from
rendered text.

## Suggested Next

Run the next supervisor-selected Step 5 packet against real prepared dispatch
fixtures for any remaining selected F128 comparison/cast/helper printer cases,
or move to the final narrow binary128 route proof if the supervisor treats this
printer surface as sufficient.

## Watchouts

- Keep final assembly printing tied to selected `MachineInstructionNode`
  records and structured q/s/d/w register operands.
- Do not infer unsupported F128 helper families, unsigned predicates, sign-bit
  operations, constants, atomics, intrinsics, or inline assembly from printer
  spelling.
- Memory-backed F128 carrier snapshots remain diagnostic-only unless a later
  packet adds a structured printable register or copy authority.
- F128 helper printing uses prepared marshal/unmarshal facts; do not replace
  those with callee-name or rendered-text guessing.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_prepared_printer)$'; } 2>&1 | tee test_after.log`

Passed, 4/4 tests. Proof log: `test_after.log`.
