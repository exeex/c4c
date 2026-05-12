Status: Active
Source Idea Path: ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Matching, Stale, And Missing Global Id Behavior

# Current Packet

## Just Finished

Completed Step 4 by recording focused proof for the selected dynamic global
scalar-array route.

Focused evidence in `tests/backend/backend_lir_to_bir_notes_test.cpp`:
- `expect_dynamic_global_scalar_array_loads_carry_link_name_id()` proves
  matching structured success despite drifted display spelling: the fixture
  renames the LIR global display name to `drifted_cases_display`, keeps the
  structured `LinkNameId` spelling as `cases`, and requires each emitted
  `bir::LoadGlobalInst` to carry `global_name == "cases"` plus the original
  `cases_id`.
- `expect_dynamic_global_scalar_array_loads_reject_missing_link_name_spelling()`
  proves the metadata-rich route fails closed when the carried `LinkNameId`
  cannot resolve to spelling text: the fixture keeps a `LinkNameId` on the
  global, detaches the link-name text table, and requires lowering to reject
  instead of falling back to raw final spelling.
- `expect_dynamic_global_scalar_array_loads_keep_no_id_compatibility()` proves
  raw/no-id compatibility remains separated: the fixture has no structured
  global id metadata, still lowers successfully, keeps `global_name == "cases"`,
  and requires `LoadGlobalInst::global_name_id == kInvalidLinkName`.

No test edit was needed because the focused Step 4 coverage already exists and
is wired into `backend_lir_to_bir_notes`.

## Suggested Next

Execute Step 5 by running the supervisor-selected validation, then record final
implementation summary, retained compatibility boundary, and green proof for
idea 194 completion review.

## Watchouts

- Do not re-prove only the addressed-global pointer route already closed by
  idea 187.
- Do not treat local route names, local slots, SSA temporaries, or block labels
  as semantic global symbols.
- The fail-closed proof covers missing/stale link-name text for a present id;
  normal public producers derive the carried id from the same `GlobalInfo` entry
  as final spelling, so an arbitrary mismatched-id fixture would need a more
  artificial consumer-level harness.
- Raw/no-id fallback remains available only when the selected access carries
  `kInvalidLinkName`; that fallback is compatibility lookup, not semantic
  global identity.

## Proof

Evidence-recording packet only; no tests or implementation files were changed,
so the delegated proof did not require a new command. Existing `test_after.log`
was left untouched.
