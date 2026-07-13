Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Repair PreparedBir publication and define the missing new-MIR handoff

# Current Packet

## Just Finished

- Step 1 review accepted the bounded inline-asm semantics but rejected formal
  adjacency: the checkpoint bypasses `PreparedBir`, while user commit
  `1a1eee652` removed the BIR-owned MIR contracts that previously supplied the
  following stage-token/verifier authority.
- Lifecycle repair now requires the inline-asm plan to be a typed component of
  verified `PreparedBir` and leaves new-MIR ownership/location undecided.

## Suggested Next

- Repair `docs/inline_asm_transport/03_schema_checkpoint.md` so Canonical BIR
  plus the ordered target-aware planners publish verified `PreparedBir`, with
  the revision/target-bound inline-asm plan as a typed component.
- Record the exact missing `PreparedBir` to new-MIR contract decisions: MIR
  owner, constructed output token, immediate verifier profile/gate,
  revision/staleness behavior, diagnostics, and transactional publication.
- Request the explicit architecture decision for that new-MIR contract's
  ownership/location; do not recreate deleted `src/backend/bir/mir/**` files.
- Repeat bounded adjacency review only after both sides of the handoff are
  authoritative. Keep Step 2 unauthorized meanwhile.

## Watchouts

- Preserve opaque payload, structured source operands, and original
  constraint/clobber syntax through Canonical BIR.
- Canonical BIR must not retain RV64 contract provenance, physical register
  interpretation, normalized classes/groups, or allocator requirements.
- Preparation must be immutable, typed, revision-bound, target-aware, and the
  sole constraint/clobber normalizer.
- Keep the RV64 preparation evidence table at `r`, `VR`, `VRM2`, `VRM4`, and
  `VRM8`; `VRM1` remains unsupported.
- The prior checkpoint re-review authorization is invalidated by the new
  architecture ownership contract.
- MIR construction may consume only verified `PreparedBir` (or a verified
  read-only view). Raw `CanonicalBir` plus an independently verified plan is
  forbidden even when their visible keys appear to agree.
- The surviving BIR README/pipeline still link deleted MIR documents. Treat
  those links as scaffold adjacency debt outside this packet, not authority.
- Do not edit the architecture scaffold, apply `stash@{0}`, or revive
  legacy BIR/prealloc/MIR/`c4c-as`.

## Proof

- Lifecycle-only repair; implementation/build proof is intentionally not run.
- Before handoff, run `git diff --check` and audit lifecycle references to
  ensure deleted MIR documents are not cited as authority and no direct
  CanonicalBir-plus-side-plan MIR boundary remains.
