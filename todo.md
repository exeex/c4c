# Current Packet

Status: Active
Source Idea Path: ideas/open/706_common_mir_named_query_migration.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Replace the Route 4 common adapter and adapt AArch64 compatibility

## Just Finished

- Step 3.2 is accepted on its dedicated common block-entry publication and
  AArch64 instruction-dispatch contracts.
- Rejected the incomplete frame-stack fixture-repair experiment and restored
  `backend_prepare_frame_stack_call_contract_test.cpp` exactly to `HEAD`.
- Waived the broader frame-stack aggregate failure for Step 3.2: it is an
  unrelated call-plan publication issue, and fixture inspection confirmed it
  is not caused by missing named call-argument relationship data.

## Suggested Next

- Start a separately bounded Step 3.3 prepared-authority family, keeping Route
  5 edge/join migration separate from placement, publication, home, move,
  freshness, frame, call-plan, and control lookup packets.

## Watchouts

- Do not treat the frame-stack aggregate failure as Step 3.2 adapter evidence;
  its call-plan publication failure predates the migrated block-entry consumer
  and belongs to a separate investigation.

## Proof

- Exact focused proof is green 2/2: `cmake --build --preset default --target
  backend_prealloc_block_entry_publications_test
  backend_aarch64_instruction_dispatch_test && ctest --test-dir build
  --output-on-failure -R
  '^(backend_prealloc_block_entry_publications|backend_aarch64_instruction_dispatch)$'
  > test_after.log 2>&1`; `test_after.log` is canonical.
