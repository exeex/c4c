# Current Packet

Status: Active
Source Idea Path: ideas/open/759_lir_typed_ref_enum_foundation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extend the enum-first pattern to related closed-set refs

## Just Finished

- Plan Step 3 complete without a header change: `LirBinaryOpcodeRef` and
  `LirCmpPredicateRef` already each provide direct enum construction, retain
  text constructors that parse once into their optional enum state, and return
  that cached state through `typed()` without reparsing rendered text.

## Suggested Next

- Step 4 proof: add focused coverage that distinguishes enum-authoritative
  builtin queries from dynamic runtime text compatibility.

## Watchouts

- Do not remove runtime string construction in 759.
- Do not start the `[[deprecated]]` migration; that belongs to 760 after 759 is
  accepted and closed.
- Do not relax verifier rules or change HIR/BIR/backend semantics to make the
  enum foundation pass.
- Dynamic vector, array, struct, function, opaque, VRM, and arbitrary integer
  spellings remain supported text-backed inputs in this foundation slice.
- Step 3 needs no API churn: both related wrappers already satisfy the
  enum-first pattern.

## Proof

- No build or test run: this packet made no code change because both wrappers
  already met the criterion. `test_after.log` was not modified.
