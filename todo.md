Status: Active
Source Idea Path: ideas/open/66_aarch64_local_dispatch_block_route.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Narrow Public Block Entry Points

# Current Packet

## Just Finished

Step 2 completed: narrowed private AArch64 dispatch route surface while
preserving public block entry points and behavior.

Changes:
- Removed unused private `dispatch.cpp` scaffolding with no call sites:
  `align_to`, `binary_uses_named_value`, `is_store_local_instruction`,
  `NarrowLocalStorePublication`, `block_context_for_label`,
  `EdgeProducerContext`, and the private
  `make_bir_machine_instruction`.
- Left `dispatch.hpp` unchanged. Public `make_block_lowering_context` and
  `dispatch_prepared_block` signatures remain available for production
  traversal and direct focused tests.
- Did not change traversal, unsupported diagnostics, hook wiring,
  before-return publication sequencing, prepared-memory retry routing,
  branch-fusion ordering, or final instruction append order.

## Suggested Next

Step 3 should keep diagnostics durable while narrowing route helpers. Suggested
packet: inspect `classify_instruction`, `append_block_diagnostic`,
`unsupported_terminator_message`, and
`append_unsupported_instruction_diagnostic`; either justify them as route-local
or narrow private helper boundaries without weakening diagnostic kind,
function/block context, instruction index, instruction family, or message text.

## Watchouts

This plan preserves the local AArch64 block route. Do not move block traversal
or target diagnostics to shared code, do not reintroduce producer/publication
or value-materialization authority into `dispatch.cpp`, and do not bundle idea
67's local-slot offset probe into this route.

Proof coverage needed for Step 3: unsupported instruction and unsupported
terminator diagnostics, missing block context, missing BIR mapping,
representative direct `dispatch_prepared_block` tests, and production traversal
through `traversal.cpp`. Do not weaken diagnostics to make cleanup pass.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_|backend_cli_aarch64_)') > test_after.log 2>&1`

`test_after.log` reports 32/32 tests passed. `git diff --check` passed.
Targeted `rg` scan confirmed the removed private names are absent from
`dispatch.cpp`.
