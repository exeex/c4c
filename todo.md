# Current Packet

Status: Active
Source Idea Path: ideas/open/786_lir_phi_special_token_semantic_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Publish and verify PHI SpecialToken authority

## Just Finished

- Switched from paused 734 Step 7.25 after preserving its accepted Steps 1
  through 7.24 and recording the upstream SpecialToken authority blocker.

## Suggested Next

- Execute Step 1 only: publish native SpecialToken semantic authority for
  relevant PHI inputs and verify it at the LIR producer boundary.

## Watchouts

- Do not change Raw-BIR/importer/receiver code or recover semantics from token
  spelling, printer output, LLVM text, labels, instruction order, or testcase
  names.
- Keep the work limited to the existing classified SpecialToken vocabulary and
  relevant PHI producers; all other operand families remain out of scope.

## Proof

- Executor: run a fresh build and focused positive/malformed LIR
  producer/verifier proof for Step 1.
- Supervisor: select and record broader/full acceptance separately.
