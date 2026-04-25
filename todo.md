Status: Active
Source Idea Path: ideas/open/107_lir_struct_name_id_type_ref_mirror.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Focused Validation

# Current Packet

Execute Step 6 from `plan.md`: run focused validation for the StructNameId
TypeRef mirror plan on the current tree and record the result.

## Just Finished

Step 6 from `plan.md` is complete for focused validation.

The delegated build plus full CTest proof passed on the current tree. No
implementation files or tests were changed in this validation-only packet.

## Suggested Next

Have the supervisor decide whether this runbook is ready for plan-owner
completion review or whether a reviewer pass is needed before lifecycle
closure.

## Watchouts

- Keep rendered `LirTypeRef` strings as printer authority.
- Parity is intentionally enforced only when a `LirTypeRef` carries a
  `StructNameId`; missing mirrors on still-string-only surfaces are not treated
  as verifier failures in this packet.
- Do not expand into globals/functions/extern signature surfaces from idea
  108.
- Do not move layout lookup to `StructNameId`; that belongs to idea 109.
- Remaining string-only follow-up surfaces: array-shaped struct fields such as
  `[N x %struct.X]`, pointer fields rendered as `ptr`, globals/functions/extern
  signature text, and other non-owned GEP sites that still only carry rendered
  type text.
- `make_lir_call_op()` still accepts return and argument type text and has no
  `StructNameTable` or `StructNameId`; callers may have structured context, but
  the helper itself does not.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log`
passed.

`test_after.log` records 2976/2976 tests passing with 0 failures.
