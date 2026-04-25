Status: Active
Source Idea Path: ideas/open/107_lir_struct_name_id_type_ref_mirror.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add TypeRef Parity Verification

# Current Packet

Execute Step 5 from `plan.md`: add verifier parity checks for `LirTypeRef`
instances carrying `StructNameId` mirrors while keeping rendered text as printer
authority.

## Just Finished

Step 5 from `plan.md` is complete for verifier-side `LirTypeRef` parity.

`verify_module()` now validates mirrored type refs through module context:
instruction type refs, structured declaration field refs, and extern declaration
return refs continue to pass the existing type-ref checks, then any attached
`StructNameId` is resolved through `mod.struct_names` and compared against the
rendered type text. Missing table entries and spelling mismatches report through
existing verifier diagnostics on the owning field.

The follow-up verifier fix for quoted or otherwise legacy-misclassified struct
spellings is in place: `StructNameId`-backed `Struct` refs may pass the
kind/text check before module parity validates the mirror, while string-only
`LirTypeRef` values retain the previous text-only validation behavior.

## Suggested Next

Have the supervisor choose the next plan step or request a reviewer pass if
Step 5 should be independently checked before more parity surfaces are added.

## Watchouts

- Keep rendered `LirTypeRef` strings as printer authority.
- Parity is intentionally enforced only when a `LirTypeRef` carries a
  `StructNameId`; missing mirrors on still-string-only surfaces are not treated
  as verifier failures in this packet.
- Do not expand into globals/functions/extern signature surfaces from idea
  108.
- Do not move layout lookup to `StructNameId`; that belongs to idea 109.
- Deferred parity surfaces: array-shaped struct fields such as
  `[N x %struct.X]`, pointer fields rendered as `ptr`, globals/functions/extern
  signature text, and other non-owned GEP sites that only have rendered type
  text in this packet.
- `make_lir_call_op()` still accepts return and argument type text and has no
  `StructNameTable` or `StructNameId`; callers may have structured context, but
  the helper itself does not.

## Proof

`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^verify_tests_' > test_after.log` passed.

`test_after.log` records 5/5 `verify_tests_` tests passing.
