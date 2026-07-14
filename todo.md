# Current Packet

Status: Active
Source Idea Path: ideas/open/759_lir_typed_ref_enum_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect existing ref wrapper authority

## Just Finished

- Plan Step 1 complete: audited `src/codegen/lir/types.hpp` authority for the
  closed-set `LirBinaryOpcodeRef` and `LirCmpPredicateRef` wrappers. Each owns
  a closed enum plus a cached optional typed value; text construction parses
  once and `typed()` returns that cache without reparsing. Enum construction
  renders through `to_string`. `LirTypeRef` remains intentionally open-ended
  (`LirTypeKind::RawText`) and separately derives integer/VRM detail from text.

## Suggested Next

- Step 2 implementation: make the closed-set wrapper enum path the explicit
  authority while retaining existing runtime string construction.

## Watchouts

- Do not remove runtime string construction in 759.
- Do not start the `[[deprecated]]` migration; that belongs to 760 after 759 is
  accepted and closed.
- Do not relax verifier rules or change HIR/BIR/backend semantics to make the
  enum foundation pass.
- `typed()` presently exposes cached parse state, so Step 2 should avoid any
  typed query path that reparses `text_`.

## Proof

- No build or test proof run: this read-only audit packet explicitly required
  none. No root-level test log was created or modified.
