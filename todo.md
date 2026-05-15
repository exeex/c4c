Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Print Supported Binary128 Machine Nodes

# Current Packet

## Just Finished

Step 5 completed the printer coverage packet for selected binary128 comparison
and cast helper boundaries. AArch64 machine printing now proves selected
`__eqtf2`, `__extenddftf2`, and `__trunctfsf2` helper paths print only from
structured q-register, scalar FP, and scalar cmp-result marshal/result records,
and the comparison printer now rejects missing `AbiCmpResultToScalar` facts
instead of printing from bare register spelling.

## Suggested Next

Run the supervisor-selected final Step 5 acceptance or broader binary128 route
proof, or hand Step 5 back to plan-owner closure if this printer surface is now
sufficient.

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
- F128 comparison printing validates the structured scalar cmp-result marshal
  record but still intentionally emits the selected `cmp`/`cset` consumption
  rather than a separate synthetic move.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_prepared_printer)$'; } 2>&1 | tee test_after.log`

Passed, 4/4 tests. Proof log: `test_after.log`.
