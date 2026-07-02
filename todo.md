Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Post-516 Register-Source Evidence

# Current Packet

## Just Finished

Lifecycle activated idea 514 after the split idea 516 multi-source
producer/classification route closed.

## Suggested Next

Execute Step 1 by reproducing the current post-516 facts for `src/pr27073.c`
and the regression-boundary facts for `src/20010518-1.c`.

## Watchouts

Keep the single register-source stack-destination owner separate from idea
516's closed multi-source producer/classification contract. Do not infer
source or destination homes from row names, source order, ABI formulas,
argument indexes, local names, or raw BIR text.

## Proof

Lifecycle-only activation. No implementation validation was run by the plan
owner.
