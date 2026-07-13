# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Resolve pipeline-wide representation choices

## Just Finished

- Plan Step 3.2 closed the pipeline's acceptance-critical representation
  choices without making the pipeline a duplicate semantic owner.
- Synchronized explicit-`Phi` SSA and the core-owned `{source BlockId,
  SuccessorRole, index}` `EdgeKey`; intrinsic registry/versioning, asm-goto
  result availability, local-unwind rejection, aggregate paths, memory-effect
  analysis ownership, and runtime-helper eligibility with their local owners.
- Selected deterministic plan-first ID reservation and cancellation/last-good
  semantics, leaving only labeled implementation choices and producer/source
  gaps in the pipeline gap ledger.

## Suggested Next

- Execute Plan Step 4, "Repair B1-B2 canonical pass identifiers."

## Watchouts

- Keep the pipeline's Section 18 as a pointer ledger: core remains sole owner
  of SSA, `EdgeKey`, intrinsic registry, asm-goto availability, and unwind
  topology; the named pass/preparation documents own their local dispositions.
- Step 4 should repair B1-B2 identifiers only and preserve the root README as
  sole normative order authority.
- Step 14 and implementation remain forbidden pending completion of the repair
  route and a new blocker-free independent Step 13 review.

## Proof

- Passed: `git diff --check && ! rg -n 'chosen canonical SSA.*must|EdgeKey.*final|intrinsic namespace.*open|asm-goto outputs.*open|exception/unwind edges.*open|runtime-helper eligibility.*open|unresolved.*(SSA|EdgeKey|intrinsic|asm-goto|exception|helper)' src/backend/bir/pipeline/README.md && rg -n 'explicit.*Phi|SuccessorRole|RegistryVersion|asm-goto|MayUnwind|runtime.helper|analysis-only|deterministic|cancellation|source gap|implementation choice' src/backend/bir/pipeline/README.md`.
- The supervisor-selected documentation proof was sufficient; this packet did
  not create or modify `test_after.log` because regression logs were explicitly
  outside packet ownership.
