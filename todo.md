# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Propagate the A-F registry through infrastructure

## Just Finished

- Plan Step 3.1 propagated the root A-F registry through the pipeline,
  canonical pass-framework, and analysis-framework contracts.
- Replaced every obsolete `S00`-`S29`/`G01` identity in the owned documents
  while preserving the `B1`-`B8` canonical interval, `C1`-`C9` preparation
  boundary, `D1`-`D5` pseudo flow, `E1`-`E4` allocation flow, and sole
  `E3 -> E1` retry edge.

## Suggested Next

- Execute Plan Step 3.2, "Resolve pipeline-wide representation choices."

## Watchouts

- Keep the root `src/backend/bir/README.md` as the sole normative order
  authority; infrastructure documents may only transcribe their owned
  intervals and exact adjacent/retry relations.
- Analyses remain immutable, non-mutating facts for exact published revisions;
  `E3` invalidates `E1` allocation facts and retries only through `E3 -> E1`.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'S([0-2][0-9]|3[0-9])|G01'
  src/backend/bir/pipeline/README.md src/backend/bir/passes/README.md
  src/backend/bir/analysis/README.md && rg -n
  'A1|A2|B1|B8|C1|C9|D1|D5|E1|E2|E3|E4|F1|root.*README|normative'
  src/backend/bir/pipeline/README.md src/backend/bir/passes/README.md
  src/backend/bir/analysis/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
