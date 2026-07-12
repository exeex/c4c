# Current Packet

Status: Active
Source Idea Path: ideas/open/729_common_current_block_join_query_exposure.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove the AArch64 routing-array reconstruction

## Just Finished

- Plan Step 2 migrated the bounded AArch64 current-block join consumers to the
  direct attached-owner common queries and deleted the routing struct, builder,
  boolean scan arrays, and private duplicate consumption query.

## Suggested Next

- Execute Plan Step 3: audit the completed boundary, run the supervisor-selected
  broader regression comparison, and prepare the lifecycle handback to idea 709.

## Watchouts

- The AArch64 wrappers now extract only stable successor/value identity; keep
  executable consumption semantics in the common attached-owner queries during
  the Step 3 audit. Three focused AArch64 tests required signature-only updates.

## Proof

- Green (4/4): `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_(aarch64_current_block_join_routing|prealloc_current_block_(lookup_attachment_lifetime|incoming_expression_authority|routed_operand_authority)))$'`.
  The supervisor-selected proof was sufficient; combined output is preserved in
  `test_after.log`.
