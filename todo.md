Status: Active
Source Idea Path: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-establish the Edge-Preservation Failure Boundary After Idea 57

# Current Packet

## Just Finished

Lifecycle activation selected Step 1 from `plan.md`; no executor packet has
run yet under this activation.

## Suggested Next

Run Step 1 to re-establish the focused edge-preservation boundary after idea 57
closure. Use the supervisor-selected focused subset that includes the
pointer-select and variadic aggregate byte-copy probes, then classify the first
bad fact before implementation proceeds.

## Watchouts

- Do not start prepared edge/terminator implementation until the post-idea-57
  first bad fact is classified.
- Do not use `00204`, `myprintf`, `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or
  `x13` as implementation selectors.
- Do not repair by reloading mutated va_list locals in the join block.
- Do not weaken expectations or mark supported probes unsupported.

## Proof

No proof has run yet for this activation.
