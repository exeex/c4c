Status: Active
Source Idea Path: ideas/open/521_bir_route8_return_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Route8 Body Boundary

# Current Packet

## Just Finished

Step 2 - Select Route8 Body Boundary is complete. The selected destination is
`src/backend/bir/bir_route8.cpp` for the existing route8 return-chain bodies,
with public route8 declarations preserved in `src/backend/bir/bir.hpp`.

Selected boundary:

- Move the route8 public definitions and their route8-local anonymous helpers
  into `bir_route8.cpp`: `route8_value_key_matches`,
  `route8_identity_matches`, `route8_key_matches`,
  `route8_return_chain_binary_opcode_is_scalar_publication`,
  `route8_is_named_value`, `route8_value_matches_name`,
  `route8_record_conflicts`, `route8_publish_return_chain_record`,
  `route8_publish_return_chain_records_for_block`,
  `route8_missing_block_status`, `route8_return_chain_value_key`,
  `route8_return_chain_record`, both `route8_build_return_chain_index`
  overloads, `route8_find_return_chain_record`,
  `route8_find_return_chain_terminal_value`, and
  `route8_find_return_chain_next_operand_value`.
- Preserve `Route8ReturnChainStatus`, `Route8ReturnChainValueKey`,
  `Route8ReturnChainRecord`, `Route8ReturnChainIndex`, and all route8 public
  function declarations in `bir.hpp`.
- Introduce only a narrow route-neutral private/shared block-match boundary for
  the existing label/id comparison currently named `route7_block_matches`.
  Step 3 should make that comparison available to both route7 code in
  `bir.cpp` and moved route8 code in `bir_route8.cpp` without exposing it as a
  public BIR API. A small private BIR helper header is acceptable for this
  exact comparison; broad route7 helper sharing is not.
- Keep route8's existing route1 dependency direction unchanged:
  `route8_return_chain_value_key` and `route8_return_chain_record` continue to
  use `route1_source_value_identity`, and route8 finders continue to return
  `Route1SourceValueIdentity`.

Rejected alternatives:

- Blindly moving route8 bodies into `bir_route8.cpp` while still calling the
  anonymous `route7_block_matches` in `bir.cpp` is unsafe because it will not
  link and hides the only non-route8 private dependency found in Step 1.
- Moving route8 declarations or records out of `bir.hpp` is rejected; public
  consumers in tests and AArch64 ALU codegen rely on the stable declaration
  surface.
- Making route8 depend on route7 public builders/finders or the route-index
  facade is rejected. The block-label/id comparison is route-neutral and should
  not create new route7/facade coupling.
- Preserving all route8 bodies in `bir.cpp` is not selected because the Step 1
  audit found route8 otherwise leaf-like, and the single private dependency can
  be isolated through the narrow block-match helper boundary without changing
  route semantics.

Step 3 implementation direction:

- Apply a behavior-preserving body extraction into `bir_route8.cpp`.
- Add only required build metadata for the new translation unit and any direct
  BIR tests that manually compile source files.
- Add or use the narrow private/shared block-match helper for the exact
  existing semantics: prefer id comparison when either side has a valid
  `BlockLabelId`, otherwise compare label text.
- Do not move route8 declarations, alter route1 scalar producer identity, touch
  route6/facade behavior, change route7 comparison semantics, or rewrite tests
  and expectations.

## Suggested Next

Suggested next packet: execute Step 3 - Apply Minimal Route8 Body Extraction Or
Preserve Decision by moving the selected route8 bodies into
`src/backend/bir/bir_route8.cpp`, adding the narrow private/shared block-match
helper boundary, and updating only required build metadata.

## Watchouts

The selected extraction relies on preserving the existing block-match semantics
exactly. Keep the helper route-neutral and private to BIR implementation files;
do not turn route8 into a route7 public-consumer or facade consumer. Route8
declarations stay in `bir.hpp`, and route1/route6/route7/facade behavior must
remain unchanged.

## Proof

No build or ctest proof was required for this decision-only packet. Local
proof: `git diff --check -- todo.md`.
