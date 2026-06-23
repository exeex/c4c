Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Residual Candidate Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00005.c`, `src/00077.c`, `src/00143.c`, and
`src/00144.c`, capturing emitted RV64 assembly and prepared BIR evidence, and
classifying the first bad runtime mechanism for each candidate.

## Watchouts

- Do not special-case candidate filenames, observed stack offsets, or source
  expression shapes.
- Do not reopen core local frame-slot address publication from idea 312 unless
  the new evidence proves it is still the first bad mechanism.
- Keep aggregate/byval and function-pointer repair out of this route.
- Split broad switch/control-flow residue into a separate idea if it becomes
  the first bad mechanism.

## Proof

Lifecycle-only activation. No build or tests were run.
