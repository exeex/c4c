# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 9.1
Current Step Title: Choose the BIR-owned D5 parallel-copy realization route

## Just Finished

- Plan Step 13 completed with four blocking architecture desynchronizations in
  `review/731_full_architecture_review_repeat.md`; the review reset returns the
  active runbook to the earliest affected repair, Step 9.1.
- Step 14 and implementation remain forbidden. The source idea stays open and
  unchanged because the reviewer found route desynchronization, not a conflict
  with its durable intent.

## Suggested Next

- Execute Plan Step 9.1, "Choose the BIR-owned D5 parallel-copy realization
  route," beginning with exact-current E1/E2/E3/realizability product ownership
  after the copy-resolution revision advance.

## Watchouts

- Repair blockers in this exact order:
  1. Step 9.1: preserve, rekey, or recompute exact-current E1 liveness, E2
     assignment, E3 spill state, and realizability after D5 copy resolution, or
     choose an alternative coherent publication route.
  2. Step 9.2: add a BIR-owned post-allocation/frame-aware realizability closure
     or schema restriction proving stack, call, spill, reload, and scratch nodes
     map one record before E4; MIR remains non-repairing.
  3. Step 9.2: separate the initial-D5 Pseudo publication verifier interval
     from the assigned E3-retry/D5-resolved private-candidate interval.
  4. Step 11: exhaustively inventory current legacy paths, including nested
     `stack_layout`, `regalloc`, and `prepared_printer` families, and correct
     address ownership from C5 to C6.
- After those repairs, repeat Step 12 and obtain a new independent Step 13
  review. Step 14 is forbidden until that review reports zero blockers.

## Proof

- Lifecycle consistency and the runbook reset must pass `git diff --check` and
  confirm that `plan.md` and `todo.md` still link the same open source idea.
- No architecture acceptance proof is claimed; the repeated review artifact is
  the authoritative blocker payload for this reset.
