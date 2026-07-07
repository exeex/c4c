Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair BIR SelectInst Object Lowering

# Current Packet

## Just Finished

Step 4 follow-up repaired RV64 object lowering for plain F32/F64
`SelectInst` predicates whose compare operands live in FPR homes. Ordinary
floating selects now materialize FPR compare predicates into a GPR condition
and branch on that condition before moving the selected FPR payload. The
`src/pr39501.c` object probe now advances past the prior `float_min1`
`SelectInst` `unsupported_instruction_fragment` and stops later at
`unsupported_call_abi` for `main` calling `float_min1`.

## Suggested Next

Delegate the next packet outside this SelectInst slice for the new
`src/pr39501.c` ordinary same-module FPR call ABI/result lowering boundary, or
return to the known `src/980604-1.c` `unsupported_terminator_fragment`
boundary if the supervisor wants to keep control-flow coverage first.

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
- The new FPR-predicate select coverage is semantic FPR compare plus FPR
  payload lowering, not a `pr39501.c` named-case shortcut.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed after the Step 4 FPR-predicate select repair. Proof log:
`test_after.log`.

Additional direct probe appended to `test_after.log`:
`build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr39501.c -o /tmp/pr39501.o`

Result: advanced beyond the prior `float_min1` `SelectInst`
`unsupported_instruction_fragment` and now stops at `unsupported_call_abi` for
`main` calling `float_min1`.
