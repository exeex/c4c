Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Repair and jointly accept the architecture checkpoint

# Current Packet

## Just Finished

- Lifecycle repair reset the active route to design review because the new
  ordered BIR scaffold moves all target-aware inline-asm normalization out of
  LIR-to-BIR/canonical BIR and into `preparation/inline_asm`.

## Suggested Next

- Repair `docs/inline_asm_transport/02_closed_constraint_contract.md` and
  `03_schema_checkpoint.md` against the user-owned architecture scaffold, then
  review the complete CanonicalBir -> preparation -> MIR adjacency.
- Do not expand this packet into review of unrelated scaffold areas; record
  architecture-wide acceptance only as the external implementation gate.
- Do not begin Step 2 implementation until both the complete ordered
  architecture and repaired checkpoint are explicitly accepted.

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
- Do not edit the architecture scaffold, apply `stash@{0}`, or revive
  legacy BIR/prealloc/MIR/`c4c-as`.

## Proof

- Lifecycle-only repair; implementation/build proof is intentionally not run.
- Required proof for this packet: lifecycle linkage, ownership wording, local
  review-state reset, and `git diff --check`.
