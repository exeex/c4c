Status: Active
Source Idea Path: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Synthetic Label Collision

# Current Packet

## Just Finished

Lifecycle switch only. Step 3 of idea 295 selected a new focused owner for
`00143` duplicate synthetic select/materialized labels, created
`ideas/open/364_aarch64_synthetic_select_label_uniqueness.md`, parked idea
295 with a durable split note, and activated this runbook.

## Suggested Next

Execute Step 1 from `plan.md`: inspect generated `00143.c.s`, map each
duplicate `.Lselect_mat_*` true/end definition to its selected node or emission
helper, and name the exact label allocation or naming boundary that reuses the
labels.

## Watchouts

Keep this distinct from idea 352 basic block label ordering and from the old
`00143` scalar-cast source-publication thread. The repair owner must be
synthetic select/materialized-label uniqueness, not a testcase-shaped label
spelling patch.

## Proof

Not run. This was a lifecycle-only switch based on the Step 2 classification
recorded in the previous idea 295 `todo.md` and `test_after.log`.
