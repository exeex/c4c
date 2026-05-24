Status: Active
Source Idea Path: ideas/open/prepared-move-publication-indexing-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Current AArch64 Prepared Indexing

# Current Packet

## Just Finished

Lifecycle activation initialized this state from
`ideas/open/prepared-move-publication-indexing-prealloc.md`.

## Suggested Next

Start with Step 1 in `plan.md`: map the current AArch64 Prepared indexing sites,
their input Prepared facts, and the target-local code that must remain in
AArch64.

## Watchouts

Keep the route limited to read-only Prepared lookup indexing. Do not move
AArch64 register, ABI, addressing-legality, or assembly-printing policy into
prealloc, and do not weaken tests or expectations.

## Proof

Lifecycle-only activation. No build or test proof required.
