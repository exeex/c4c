# Store Source Publication Dump Visibility

## Goal

Expose bounded store-source publication-plan facts in prepared-module dump
output so source-producer and direct-global select-chain dependencies are
reviewable by contract tests.

## Why This Exists

The select-chain dump contract closure deferred store-source dump visibility
because that route had not added a bounded prepared-module store-source
carried fact. The residue audit found that current code now carries bounded
store-source facts in `PreparedStoreSourcePublicationPlan`: source-producer
kind, source block and instruction coordinates, producer instruction pointers,
direct-global select-chain source, root-is-select state, and root instruction
index.

Those facts are consumed by target-side planning, especially AArch64 store
local/global publication paths, but `prepare::print()` has no store-source
publication-plan section. The remaining gap is contract visibility, not a new
BIR semantic authority.

## In Scope

- Add a narrow prepared-printer/module dump surface for store-source
  publication plans.
- Print source-producer fields and direct-global select-chain fields already
  present on `PreparedStoreSourcePublicationPlan`.
- Add focused prepared-printer tests that prove visible store-source
  source-producer and direct-global select-chain dependency rows.
- Preserve fail-closed behavior when store-source publication inputs are
  missing or incomplete.
- Keep naming and formatting consistent with existing select-chain and call
  argument dump sections.

## Out Of Scope

- Inventing a new BIR semantic source-producer fact when existing
  `PreparedEdgePublicationSourceProducer` and direct-global select-chain
  queries already feed the plan.
- Broad prepared-printer expansion unrelated to store-source publication-plan
  visibility.
- Rewriting AArch64 store-source lowering behavior except where tests need a
  stable fixture.
- Treating `select_chain_lookups.cpp` helper authority as the problem; current
  audit classifies it as safe query glue.
- Creating x86 or RISC-V implementation work.

## Acceptance And Completion Criteria

- Prepared dump output includes a bounded store-source publication-plan section
  or equivalent rows that expose source-producer and direct-global select-chain
  fields for store-source plans.
- Prepared-printer tests assert the new store-source visibility surface.
- Existing store-source planning tests continue to show producer fields,
  direct-global select-chain fields, and missing-input fail-closed behavior.
- The output does not expose raw unrelated implementation pointers or broaden
  the dump into unrelated publication-plan internals.
- No backend or printer expectation is weakened to avoid the visibility
  contract.

## Ownership And Proof Route

Owner: prealloc prepared-printer/module dump owns the visibility surface;
publication planning owns only the already-existing
`PreparedStoreSourcePublicationPlan` facts if a missing printed field requires
minor structured access.

Proof route:

- Inspect `prepare::print()` and `src/backend/prealloc/prepared_printer/` for
  the existing call-plan and select-chain dump conventions.
- Add focused prepared-printer coverage in
  `tests/backend/bir/backend_prepared_printer_test.cpp` or the nearest
  existing prepared-printer test bucket.
- Re-run focused store-source publication-plan tests that already validate
  planning facts.
- Run `git diff --check` and the supervisor-selected regression subset before
  accepting the visibility work.

## Reviewer Reject Signals

- The change creates broad printer output for unrelated publication internals
  instead of the bounded store-source fields.
- The implementation re-derives source-producer or select-chain facts in the
  printer rather than printing existing prepared/publication-plan facts.
- Tests only check that a header exists and do not assert source-producer or
  direct-global select-chain fields.
- The work weakens store-source fail-closed behavior or rewrites expectation
  contracts to avoid missing visibility.
- The idea is expanded into architecture-specific x86/RISC-V implementation
  before the shared dump contract exists.
