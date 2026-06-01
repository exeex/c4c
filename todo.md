Status: Active
Source Idea Path: ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Proof

# Current Packet

## Just Finished

Step 3: added focused backend dispatch proof for the same-width I32
stack-source edge-copy path migrated in Step 2.

`same_width_i32_stack_source_edge_copy_uses_prepared_typed_stack_source` builds a
prepared predecessor-to-join edge where the source value has a concrete I32
stack-slot home and the destination has prepared GPR placement authority. The
test asserts `prepare_same_width_i32_stack_source_publication` is available and
that predecessor dispatch emits `ldr w13, [sp, #64]` into the prepared
destination GPR before the branch, without materializing the producer expression
or emitting a fallback move.

## Suggested Next

Run supervisor review/acceptance for Step 3 or delegate the next plan step if
the current proof slice is accepted.

## Watchouts

- Do not thread `PreparedEdgePublication` into `dispatch_publication.cpp` just
  to satisfy this route.
- Do not add named testcase shortcuts or weaken expected supported behavior.
- Keep AArch64 load/copy/register emission target-local.
- The new fast path intentionally accepts only `Available` same-width I32
  typed stack-source facts; every other status falls back to the existing
  recursive publication route.
- The focused test uses a prepared caller-saved placement that maps to `w13` so
  it proves prepared destination placement authority rather than only a textual
  register name.

## Proof

Passed:

`cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_instruction_dispatch$' >> test_after.log`

`test_after.log` contains the delegated proof. Targeted subset result:
1/1 test passed.
