Status: Active
Source Idea Path: ideas/open/64_aarch64_join_parallel_copy_prepared_query.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Current-Block Join-Copy Authority

# Current Packet

## Just Finished

No executor packet has run for this plan yet.

## Suggested Next

Start Step 1 by inventorying `build_current_block_join_parallel_copy_cache`, its
raw prepared-home and move-bundle inputs, its current-block join source facts,
and every dispatch hook consumer. Record the inventory and any shared prepared
query gaps here before implementation edits.

## Watchouts

Do not turn the old AArch64 cache into a renamed local semantic authority. Keep
branch-fusion sequencing, before-return publication ordering, same-block scalar
recursion, edge fallback, and select-chain dependency discovery out of scope.

## Proof

No proof has run yet. Activation is lifecycle-only.
