Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair BIR SelectInst Object Lowering

# Current Packet

## Just Finished

Step 4 first packet repaired RV64 object lowering for plain floating
`SelectInst` materialization and for prepared join-transfer select carriers
whose published edge copies place the selected value in an FPR register home.
Ordinary F64 selects now materialize local branch/jump control flow, move FPR
payloads, and store stack-home results without weakening select-carrier alias
or join-transfer classification. The `src/980604-1.c` object probe now
advances past the previous `SelectInst` `unsupported_instruction_fragment`
blocker and stops at the next `unsupported_terminator_fragment` boundary.
`src/pr39501.c` still stops at a `SelectInst` object-lowering blocker and
needs a follow-up Step 4 packet.

## Suggested Next

Delegate the next Step 4 follow-up packet for the remaining `src/pr39501.c`
`float_min1` `SelectInst` object-lowering shape before chasing the new
`src/980604-1.c` `unsupported_terminator_fragment` boundary.

## Watchouts

- Keep BIR scalar-control-flow producer work in
  `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- Keep function-signature, scalar-binop, and scalar/local-memory work in their
  own open ideas.
- Do not use row reclassification, expectation rewrites, unsupported markers,
  allowlist edits, or named torture-case shortcuts as progress.
- The Step 4 change treats prepared FPR join-transfer select carriers with
  published edge copies like existing GPR register-home carriers: the select
  instruction is skipped only after prepared object consumer classification and
  published-copy checks succeed.
- The new ordinary F64 select coverage is semantic FPR payload lowering, not a
  testcase-shaped `980604-1.c` shortcut.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed after the Step 4 first-packet repair. Proof log:
`test_after.log`.

Additional direct probe appended to `test_after.log`:
`build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/980604-1.c -o /tmp/980604-1.o`

Result: advanced beyond the Step 4 `SelectInst`
`unsupported_instruction_fragment` and now stops at
`unsupported_terminator_fragment`.

Additional supervisor probe:
`build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr39501.c -o /tmp/pr39501.o`

Result: still stops at `unsupported_instruction_fragment` for
`float_min1` `SelectInst`; keep Step 4 active for that follow-up.
