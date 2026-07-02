Status: Active
Source Idea Path: ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct The 43-Row Prepared Authority Queue

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1,
"Reconstruct The 43-Row Prepared Authority Queue." No implementation work has
started.

## Suggested Next

Executor should run Step 1 as a proof/accounting packet: reconstruct the
deduplicated prepared-authority queue, group rows by earliest missing authority
family, identify representative rows and prepared/module surfaces to inspect,
and record the supervisor-selected proof command.

## Watchouts

- Do not infer prepared facts from RV64 destination spelling or expected
  assembly.
- Keep the 12 carry-in rows from the closed 551 materialization lane inside
  this queue unless row-level evidence reroutes them to an earlier owner.
- Do not edit implementation files unless the delegated packet explicitly
  includes a code change.

## Proof

- Lifecycle-only activation; no build proof required before executor work.
