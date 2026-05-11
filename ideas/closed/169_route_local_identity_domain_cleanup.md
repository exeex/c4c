# Route-Local Identity Domain Cleanup

Status: Closed
Created: 2026-05-11
Closed: 2026-05-11

Depends On:
- `ideas/closed/167_whole_codebase_string_authority_final_audit.md`

Parent Ideas:
- `ideas/closed/162_link_name_id_backend_symbol_authority.md`

## Goal

Clean up route-local identity domains that legitimately should not use
source-level `TextId` or link-visible `LinkNameId`, but also should not rely on
ambiguous raw strings where a local id domain is more precise.

This covers compiler-internal local names such as SSA values, block labels,
local slots, prepared backend value names, register-allocation handles, and
temporary address/provenance names.

## Why This Idea Exists

The string authority cleanup series separates semantic identity from rendered
text. Some strings are not source or linker identity at all: they are
function-local, route-local, or backend-stage-local handles. Those should not
be forced into `TextId` or `LinkNameId`, but they may deserve their own typed
id tables when they participate in lookup, validation, or cross-pass
transport.

Idea 167 should identify which route-local strings are harmless display and
which are acting as local semantic keys.

## In Scope

- Inventory route-local string keys in LIR, BIR, prealloc, MIR-prep, and shared
  backend helpers.
- Classify each as display-only, generated local handle, validation key,
  cross-pass local identity, or compatibility payload.
- Introduce or reuse typed local ids where a route-local string is used as
  lookup authority across multiple structures.
- Preserve final printed spelling by rendering from the local id table or the
  existing local name payload.
- Add validation checks that route-local ids, not display spellings, connect
  related local artifacts where feasible.
- Keep source `TextId` and `LinkNameId` reserved for source/link-visible
  domains.

## Out Of Scope

- Parser/sema/HIR source identity cleanup.
- Link-visible symbol cleanup already handled by idea 162.
- Renaming final emitted labels/registers for cosmetic reasons.
- Rewriting the whole backend allocation pipeline.

## Candidate Areas

- LIR value names, stack object names, block labels, and string-pool names.
- BIR `Value::name`, local slots, block labels, phi incoming labels, and
  route-local memory-address base names.
- Prealloc prepared value names, slot pointer carriers, register assignment
  names, and temporary addressing handles.
- Any local string map that validates or connects two route-local artifacts.

## Acceptance Criteria

- Route-local string keys are classified separately from source and link-name
  identity.
- At least one meaningful route-local lookup family is moved to typed local id
  authority or explicitly fenced as display-only.
- Output spelling remains stable unless an intentional rendering change is
  documented.
- Tests cover id-based route-local lookup or validation, not just printed text.

## Closure Notes

Idea 169 completed after the Step 5 broader backend checkpoint. The runbook
selected BIR local-slot authority as the bounded route-local lookup family and
migrated it to typed `SlotNameId` authority for BIR local slots, local
load/store references, local-slot memory-address bases, and sret storage while
preserving rendered spelling for display and no-id compatibility payloads.

Validation included direct verifier coverage for duplicate ids,
load/store/address id-name mismatches, and no-id compatibility, plus a broader
backend checkpoint:

```bash
cmake --build build -j
ctest --test-dir build -j --output-on-failure -R '^(backend_|backend_codegen_route_|positive_sema_inline_backend_coord_c$)'
```

The checkpoint passed with 110/110 runnable tests; 12 disabled MIR trace tests
did not run. Remaining route-local string families were classified. AArch64
direct LIR emitter string-keyed lowering and the larger prealloc/out-of-SSA
raw-name helper set are follow-up initiative candidates if deeper typed-id
migration is needed. Idea 170 continues to own string-authority guard and
allowlist machinery.

## Reviewer Reject Signals

- The work tries to use `TextId` or `LinkNameId` for route-local temporary
  domains.
- Display-only local names are refactored without semantic benefit.
- A lookup still keys by rendered route-local spelling after an id exists.
- Tests only check cosmetic output.
