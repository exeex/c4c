Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Register-To-Stack Move Materialization

# Current Packet

## Just Finished

Attempted Step 2, `Implement Register-To-Stack Move Materialization`.

Completed Step 2, `Implement Register-To-Stack Move Materialization`.

`src/backend/mir/riscv/codegen/object_emission.cpp` now consumes prepared
register-home to stack-slot-home moves semantically for the selected
before-instruction move-bundle path:

- removes the single-register-to-stack-move restriction so multi-move bundles
  can be emitted move-by-move
- keeps explicit source scalar-size authority, source GPR home coherence, and
  destination frame-slot storage-plan coherence checks
- stores by the prepared destination value type and stack-slot offset instead
  of requiring source and destination scalar sizes to match
- handles coherent rematerializable integer immediate sources to stack slots
  when they appear in the same stack-destination bundle, using an unoccupied
  temporary GPR and the existing rematerializable-immediate verifier

The original two-move representative `src/20000717-3.c` is no longer a clean
Step 2 proof case: after the selected register-to-stack bundle advances, it
reaches a later move bundle whose move says `destination_storage=stack_slot`
but whose prepared destination home is `rematerializable_immediate`. RV64 has
no explicit destination stack-slot home or stack offset authority to consume
for that later bundle.

Replacement representative: `src/20000914-1.c`. The classification table keeps
it in the same 130-row selected first packet with
`move_count=2`, `source_home_kind=register`, and
`destination_home_kind=stack_slot`, and the revised proof confirms it no longer
ends at generic move-bundle materialization.

## Suggested Next

Proceed to Step 3 immediate-source move coverage, keeping the
`src/20000717-3.c` later `destination_home_kind=rematerializable_immediate`
authority gap out of the RV64 materialization route until prepared authority is
clarified.

## Watchouts

- Do not make RV64 infer a stack slot from
  `destination_home_kind=rematerializable_immediate`; the discovered
  `src/20000717-3.c` residual is missing prepared destination authority after
  Step 2 advances its earlier selected register-to-stack bundle.
- The current code changes are semantic over prepared homes and do not use
  filenames, expectation rewrites, unsupported-marker edits, or allowlist
  changes.
- `src/20000422-1.c` and `src/20000914-1.c` cover multi-move behavior in the
  revised Step 2 proof set.

## Proof

- Revised delegated proof command was run with `src/20000914-1.c` replacing
  `src/20000717-3.c`; full output is preserved in `test_after.log`.
- `cmake --build --preset default` completed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345 backend tests.
- The five-case RV64 gcc torture allowlist advanced all representatives past
  `fragment_status=generic_move_bundle_materialization_failed`:
  `src/pr78438.c`, `src/20000121-1.c`, `src/20000801-2.c`, and
  `src/20000422-1.c`, and `src/20000914-1.c`.
- The final assertion reports `generic_move_bundle_failure_count=0`.
