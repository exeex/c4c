Status: Active
Source Idea Path: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Prepared Move-Bundle And Select Publication Lowering

# Current Packet

## Just Finished

Step 3 repaired the prepared move-bundle/select-publication object-lowering
shape exposed by `src/980604-1.c`. RV64 object emission now admits FPR-to-FPR
select-publication moves, lowers FPR out-of-SSA phi moves, and materializes the
edge-preserved floating zero-compare condition from prepared branch-condition
facts when raw successor BIR is not instruction-bearing in the object route.
Malformed or ambiguous bundles still fail closed with the existing
`unsupported_move_bundle_target_shape` diagnostic.

## Suggested Next

Delegate Step 4 from `plan.md`: repair plain BIR `SelectInst` RV64 object
lowering for the next `src/980604-1.c` / `src/pr39501.c` blocker without
weakening select-carrier alias or join-transfer contracts.

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
- The Step 3 repair only covers select-publication/FPR move-bundle emission
  and edge-preserved floating zero compares; it does not repair plain
  `SelectInst` object lowering.

## Proof

Ran:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: passed after the Step 3 repair. Proof log: `test_after.log`.

Supervisor follow-up:
`build/c4cll -I /workspaces/c4c --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/980604-1.c -o /tmp/980604-1.o`

Result: advanced beyond the previous
`unsupported_move_bundle_target_shape` blocker and now stops at the Step 4
`unsupported_instruction_fragment` / `SelectInst` object-lowering boundary.
