Status: Active
Source Idea Path: ideas/open/135_shared_current_block_entry_publication_query.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Rediscovery Site

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and initialized execution state
for Step 1.

## Suggested Next

Inspect the AArch64 current-block publication and value-home lookup paths, then
select the concrete rediscovery site and shared prepared/prealloc query owner.

## Watchouts

- Do not move AArch64 register-view selection, publication register recording,
  branch-fusion hooks, relocation operands, or final instruction spelling into
  shared code.
- Do not replace prepared facts with local scans or BIR-name matching behind a
  new API name.
- Use matching `test_before.log` and `test_after.log` for acceptance proof.

## Proof

Lifecycle-only activation; no build or backend tests run.
