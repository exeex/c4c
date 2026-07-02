Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Rematerializable Immediate Move Coverage

# Current Packet

## Just Finished

Completed Step 3, `Add Rematerializable Immediate Move Coverage`.

`src/backend/mir/riscv/codegen/object_emission.cpp` now consumes coherent
rematerializable integer-immediate sources for register destinations in the
selected move-bundle paths:

- block-entry out-of-SSA `phi_join_register_to_register` moves whose prepared
  source home is `RematerializableImmediate` now verify the prepared immediate
  contract and load the immediate into the prepared destination GPR
- predecessor select-publication parallel copies with an available I32
  immediate source and explicit GPR destination now emit through the existing
  RV64 load-immediate helper, including wider-than-12-bit immediates
- prepared-authority gaps remain outside this route; the code requires the
  published select intent and matching predecessor-terminator parallel-copy
  authority instead of inferring missing homes

Immediate-to-stack coverage from Step 2 remains in place for coherent
stack-destination bundles and was included in the Step 3 representative proof.

## Suggested Next

Proceed to Step 4 stack-to-stack move coverage for coherent
`consumer_stack_to_stack/stack_slot_to_stack_slot` rows.

## Watchouts

- Do not make RV64 infer a stack slot from
  `destination_home_kind=rematerializable_immediate`; the discovered
  `src/20000717-3.c` residual is missing prepared destination authority after
  Step 2 advances its earlier selected register-to-stack bundle.
- The current code changes are semantic over prepared homes and do not use
  filenames, expectation rewrites, unsupported-marker edits, or allowlist
  changes.
- Step 3 representatives advance past the targeted move-bundle/select
  publication failures, but several continue to later unsupported features:
  `src/20080519-1.c` reaches unsupported local-memory addressing,
  `src/pr29695-1.c` and `src/pr29695-2.c` reach missing move-bundle authority,
  and `src/pr37924.c` reaches unsupported instruction lowering.

## Proof

- Delegated Step 3 proof command was run exactly; full output is preserved in
  `test_after.log`.
- `cmake --build --preset default` completed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345 backend tests.
- The five-case RV64 gcc torture allowlist was
  `src/20080519-1.c`, `src/20060102-1.c`, `src/pr29695-1.c`,
  `src/pr29695-2.c`, and `src/pr37924.c`.
- The final assertion reports `generic_move_bundle_failure_count=0` and
  `select_publication_failure_count=0`.
