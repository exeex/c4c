Status: Active
Source Idea Path: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Producer-Side Authority Or Narrow The Diagnostic

# Current Packet

## Just Finished

Step 4 narrowed the unchanged representative authority failure at the RV64
prepared consumer surface.  The Step 4 prepared dump showed non-parallel
`before_instruction` / `authority=none` move bundles with two distinct register
sources targeting the same stack-slot destination; no ordered, parallel-copy,
select-backed, or mutually-exclusive authority was present in the bundle.  The
RV64 object path now reports a specific
`producer_authority_missing_for_register_fan_in_stack_destination` diagnostic
for that two-register-source stack-destination shape while preserving the
shared `ambiguous_non_parallel_multi_source_stack_destination` category.

## Suggested Next

Proceed to Step 5: rerun the representative RV64 object route for
`src/20000605-1.c` and save the result under the Step 5 agent-state path to
confirm the old generic ambiguity text has either become the narrower
stack-destination authority diagnostic or advanced to a later owner.

## Watchouts

- Keep this mapped to
  `ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md`.
- Do not bypass the prepared move-bundle classifier broadly or key behavior to
  `src/20000605-1.c`, `render_image_rgb_a`, temporary names, or wrapper text.
- The earliest shared classifier lives under `src/backend/prealloc/`, which was
  outside this packet's owned files; this slice keeps the narrower message in
  the owned RV64 prepared consumer return path without changing the shared enum
  category.
- Focused coverage keeps a three-move genuinely ambiguous stack-destination
  bundle on the old generic fail-closed diagnostic.

## Proof

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`
passed; proof log: `test_after.log`.
