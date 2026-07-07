Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Prepared Terminator Object Lowering

# Current Packet

## Just Finished

Step 2 repaired the prepared terminator object-lowering shape exposed by the
register-condition representatives. RV64 object emission now lowers a prepared
named GPR truth-value condition as `bne condition, zero, true` followed by a
local jump to the false successor, while missing homes, stack homes, and
floating condition values remain fail-closed with the existing
`unsupported_terminator_fragment` diagnostic.

## Suggested Next

Delegate Step 3 from `plan.md`: inspect the `src/980604-1.c`
prepared move-bundle/select-publication target shape and repair the semantic
RV64 move or admission path without weakening ambiguous or unsupported bundle
rejections.

## Watchouts

- Keep BIR scalar-control-flow producer work in
  `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`.
- Keep function-signature, scalar-binop, and scalar/local-memory work in their
  own open ideas.
- Do not use row reclassification, expectation rewrites, unsupported markers,
  allowlist edits, or named torture-case shortcuts as progress.
- Clearing fused compare `branch_conditions` on the focused fixture is now a
  supported unfused register-condition branch shape, not a malformed prepared
  fact. Malformed condition coverage should mutate the condition home or type.
- The Step 2 repair only covers already-homed scalar/pointer GPR condition
  values; it does not repair move bundles, select publication, or plain
  `SelectInst` object lowering.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed after the Step 2 repair. Proof log: `test_after.log`.

Supervisor follow-up:
`ctest --test-dir build -j --output-on-failure -R 'llvm_gcc_c_torture_src_(20000314_3|930614_1|pr35456)_c'`

Result: passed; the Step 2 terminator representatives now advance beyond the
previous `unsupported_terminator_fragment` blocker.
