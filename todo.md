# Current Packet

Status: Active
Source Idea Path: ideas/open/729_common_current_block_join_query_exposure.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the direct common query seam

## Just Finished

- Plan Step 1 established target-neutral attached-owner queries for incoming-
  expression and source roles using stable successor/value identity, while
  preserving missing, stale, incomplete, unsupported, mismatched, ambiguous,
  and disagreeing fail-closed results.

## Suggested Next

- Execute Plan Step 2: migrate the bounded AArch64 consumers to the direct
  attached common queries and delete the target-built routing array.

## Watchouts

- Preserve the common seam's exact owner/attachment identity check. Do not
  replace stable prepared value identity with Route 5, instruction position,
  or target-local result-name matching during the AArch64 migration.

## Proof

- Green: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_(aarch64_current_block_join_routing|prealloc_current_block_(lookup_attachment_lifetime|incoming_expression_authority|routed_operand_authority)))$'`.
  Combined output is preserved in `test_after.log`.
