Status: Active
Source Idea Path: ideas/open/715_pass_ready_bir_schema_and_legacy_quarantine_research.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Fields And Define Legacy Quarantine

# Current Packet

## Just Finished

- Completed plan.md Step 2: classified every Step 1 field/family exactly once
  and specified the observational `LegacyBirCompatibilityCapsule`, closed
  reader allowlist, authority boundaries, and zero-field/zero-reader gates.

## Suggested Next

- Execute plan.md Step 3 and define stable IDs, mutation APIs, verification,
  recomputable analyses, invalidation, and module/function ownership.

## Watchouts

- Preserve the registry's field-level split for mixed current structs; do not
  migrate a whole struct into two authoritative destinations.
- Capsule counts must decrease component-wise and end at zero fields and zero
  readers; renaming or coalescing manifest keys is not deletion progress.

## Proof

- Supervisor-selected documentation proof passed:
  `git diff --check && test -f docs/backend/pass_ready_bir/02_field_classification_and_quarantine.md && rg -n "future core IR|recomputable analysis|lowering-only input|prepared/MIR output|legacy compatibility/debug|LegacyBirCompatibilityCapsule|allowlist|zero fields|zero readers" docs/backend/pass_ready_bir/02_field_classification_and_quarantine.md`.
- This documentation-only packet does not produce `test_after.log`; no build or
  test subset was delegated.
