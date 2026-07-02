# Pointer-Value Memory Provenance Publication

Status: Closed
Type: Producer publication and opaque-compatibility policy idea
Parent: `ideas/closed/438_prepared_pointer_value_memory_authority.md`
Source Evidence: `build/agent_state/438_step1_pointer_value_memory_audit/`
Owning Layer: BIR/prepared pointer-value memory provenance publication before RV64 target consumption

## Current Lifecycle State

Closed after Step 5. The internal-only same-module computed-address formal
pointer provenance packet is complete and valid. The external-linkage
`930930-1::f` representative remains intentionally unsupported because
observed same-module direct calls are not complete authority for an
external-linkage callee formal, and ideas 443, 444, and 445 did not produce a
sound no-external-caller metadata source.

Residual pointer-value memory accesses are classified as intentionally
unsupported under the prepared authority contract. Pointer-delta propagation,
including `%mr_TR - 8`, remains later work only after base formal pointer
provenance is authorized.

Close proof used the focused backend regression guard scope:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

`test_before.log` and `test_after.log` both covered the same backend scope with
`345/345` tests passing and no new failures under the non-decreasing
maintenance/lifecycle guard mode. Broader/full-suite baseline acceptance was
not used for this close because it is blocked by the separate reproducible
`string_authority_guard` failure in
`src/backend/bir/lir_to_bir/module.cpp`.

## Goal

Publish or deliberately reject stronger pointer-value memory provenance for
runtime pointer dereferences so target lowering only consumes explicit prepared
authority.

## Why This Exists

Idea 438 defined and covered the prepared authority contract with
`prepare::prepared_pointer_value_memory_has_proven_authority`, but `930930-1`
still emits pointer-value memory records with `layout_authority=unknown` and
`range_verdict=unknown_compatible`. That is the correct fail-closed result for
the contract slice, but it leaves a separate producer question: whether the
underlying runtime pointer patterns can receive concrete provenance/layout/range
facts, need a named opaque-compatibility policy, or must remain unsupported.

## In Scope

- Re-audit `930930-1` pointer-value memory records against the classifier from
  idea 438.
- Determine whether runtime pointers such as `reg1` and `mr_TR - 8` can be
  tied to concrete prepared provenance, complete object extent, and
  `ProvenInBounds` ranges.
- Define any narrow `OpaqueCompatibility` policy explicitly, with focused
  coverage, if concrete provenance is impossible but the route is still
  semantically owned.
- Create a later RV64 target-consumer packet only after prepared records pass
  the proven-authority classifier or a separately covered opaque policy.
- Preserve fail-closed diagnostics for unknown layout, unknown-compatible
  ranges, bare pointer identities, and unsupported pointer arithmetic.

## Out Of Scope

- Redefining the authority contract or weakening
  `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Inferring pointer ownership, object extent, layout, or range in RV64 from raw
  BIR, testcase names, value names, integer arithmetic, or object labels.
- Store-source/global-memory publication, direct-global return/select-chain,
  and terminator/select publication work.
- Reopening pointer-cast movement from idea 429 or selected global object data
  from idea 433.
- Expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes.

## Acceptance Criteria

- Each `930930-1` pointer-value memory access is classified as concrete
  provenance, explicitly accepted opaque compatibility, or intentionally
  unsupported, with representative evidence.
- Any accepted route produces prepared records that pass the idea 438
  classifier or a newly named opaque-compatibility predicate with focused tests.
- Missing/incoherent pointer provenance remains fail-closed.
- Any RV64 target-consumer work is split or sequenced after producer authority
  is proven, not bundled with producer inference.

## Reviewer Reject Signals

- Reject RV64 changes that infer pointer-value memory provenance, layout,
  range, or object extent from raw pointer values, integer casts, filenames,
  function names, value names, or object labels.
- Reject weakening `prepared_pointer_value_memory_has_proven_authority` so
  `layout_authority=unknown` or `range_verdict=UnknownCompatible` silently
  becomes target-consumable.
- Reject testcase-shaped fixes keyed to `930930-1`, `reg1`, `mr_TR`, one block,
  or one dump layout.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as provenance publication progress.
- Reject broad memory-provenance rewrites that hide the old unknown-authority
  failure behind a new helper name.
