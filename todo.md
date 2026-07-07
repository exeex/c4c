Status: Active
Source Idea Path: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Representative Route

# Current Packet

## Just Finished

Completed Step 3 classifier-side narrowing for the prepared stack-destination
authority path.

`prepared_move_bundle_has_ambiguous_multi_source_stack_destination` now treats
the ambiguous non-parallel multi-source rejection as a normal value-to-stack
move authority check: both candidate moves must be ordinary `Move` records,
target value destinations, publish stack-slot destination storage, have no
cycle-temp or immediate source, and carry no independent move authority. The
valid supported register-to-stack behavior remains accepted, and the Step 2
binary/compare two-register-source fixture still rejects with
`AmbiguousNonParallelMultiSourceStackDestination`.

Added a metadata-mismatch guard to
`rejects_ambiguous_non_parallel_multi_source_stack_destination_move_bundle` so
same-destination multi-source records that do not actually publish stack-slot
destination storage fall through to the existing generic move-bundle shape
diagnostic instead of being mislabeled as the stack-destination authority
ambiguity.

## Suggested Next

Step 4 should run the representative-route handoff: verify the original
binary/compare route that motivated the ambiguity now fails closed with the
classifier-owned diagnostic, while keeping nearby valid stack-destination move
bundle materialization accepted.

## Watchouts

- The 574 FP binary acceptance criteria are complete; do not fold this
  move-bundle authority work back into FP binary lowering.
- Do not bypass the prepared move-bundle classifier broadly. The representative
  first blocker is currently a genuine ambiguity in prepared authority, not a
  valid ordered stack-destination move sequence.
- Valid stack-destination multi-source materialization should stay tied to
  select or parallel-copy authority, not filename/function/value-name matching.
- The Step 3 implementation deliberately did not rewrite producer-side
  publication. It narrowed classifier ownership so malformed non-stack
  destination records keep their existing RV64 shape diagnostics, while true
  non-parallel multi-register-source stack-destination bundles remain
  classifier rejections.

## Proof

Delegated proof passed; log path `test_after.log`.

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1
```

Result: build succeeded; `backend_riscv_object_emission` passed.
