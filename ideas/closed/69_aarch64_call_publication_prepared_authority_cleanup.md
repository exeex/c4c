# AArch64 Call And Publication Prepared Authority Cleanup

## Completion Notes

Closed after Step 5 consolidation. AArch64 call lowering now consumes prepared
call-boundary effect authority before target-local register rendering,
instruction spelling, and machine-record construction. The migrated
publication surface consumes prepared same-block and fused-compare publication
source producers, and fixed-formal store-local publication consumes the
prepared store-source plan before target-local store emission.

The only remaining typed stack-source publication residue is outside this
idea's owned files and is tracked separately in
`ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md` for
the `dispatch_edge_copies.cpp` owner.

Close proof: backend regression guard over `^backend_` passed with
`test_before.log` and `test_after.log` both at 169/169 passing tests, using
the documented non-decreasing mode for lifecycle-only closure.

## Goal

Make AArch64 call and publication lowering consume the prepared call,
move-bundle, after-call result, and publication source facts directly, leaving
only AArch64 register conversion, copy instruction selection, call spelling,
and machine-record construction in target-local code.

## Why This Exists

The large-owner residue audit classified call-plan lookup, argument/result
binding, call-boundary move bundles, edge publication, edge-copy source facts,
current-block join sources, typed stack-source publication, and store-source
publication as `consume-shared`. Existing prepared authority already names the
facts AArch64 should be using; the cleanup should remove local re-derivation
rather than create another target-local policy layer.

## Owned Files

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- Shared prepared authority call sites only where needed to expose or consume
  existing facts:
  - `src/backend/prealloc/call_plans.*`
  - `src/backend/prealloc/calls.hpp`
  - `src/backend/prealloc/prepared_lookups.*`
  - `src/backend/prealloc/publication_plans.*`

## In Scope

- Replace local call-plan, argument, result-lane, and call-boundary move-bundle
  re-derivation with `PreparedCallPlan`, `PreparedCallArgumentPlan`,
  `PreparedMoveBundle`, and `PreparedAfterCallResultLaneBinding` queries.
- Route publication source selection through `PreparedEdgePublication*`,
  `PreparedEdgeCopySourceFacts`,
  `PreparedCurrentBlockJoinParallelCopySourceFacts`,
  `PreparedEdgePublicationSourceProducer*`,
  `PreparedTypedStackSourcePublication`, and prepared store-source publication
  plans.
- Preserve target-local AArch64 register conversion, scratch selection,
  direct/indirect call spelling, copy/move instruction selection, and
  call-boundary machine records.
- Add or adjust narrow tests that prove AArch64 consumes prepared call and
  publication facts across call edges, edge copies, typed stack sources, and
  store-source publication.

## Out Of Scope

- Rewriting the shared call-plan or publication-plan model when the needed fact
  already exists.
- Moving AArch64 ABI register conversion or instruction spelling into shared
  prealloc code.
- Reopening dispatch-family contraction as a broad file-size cleanup.
- Changing supported behavior, downgrading unsupported expectations, or
  weakening diagnostics to make the cleanup appear green.

## Proof Expectations

- A focused build or CTest subset covering AArch64 call lowering,
  call-boundary publication, dispatch publication, edge-copy publication, and
  typed stack-source publication.
- Before/after regression logs when the supervisor treats the slice as an
  acceptance candidate.
- Tests or dumps must show the same prepared facts drive the AArch64 records;
  line-count reduction alone is not proof.

## Reviewer Reject Signals

- The route adds named-case shortcuts for one call or publication testcase
  instead of consuming prepared call/publication facts.
- The diff rewrites expectations or marks a supported path unsupported without
  explicit user approval.
- A helper is renamed or moved while still locally re-deriving call-plan,
  move-bundle, edge-source, or typed-stack-source authority.
- Shared prealloc starts owning AArch64 register conversion, call spelling, or
  concrete copy instruction selection.
- Broad dispatch-family rewrites are mixed into this call/publication cleanup.
