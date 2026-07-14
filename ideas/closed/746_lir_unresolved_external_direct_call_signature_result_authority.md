# Unresolved External Direct Scalar-Call Signature and Result Authority

Status: Closed (corrected route accepted; capability delivered through normal Function authority)
Type: producer-side unresolved-external call authority prerequisite
Blocks: Plan Step 7.32 of ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Related research prerequisite: ideas/closed/747_frontend_source_representation_for_unresolved_external_direct_calls.md

## Goal

Give the production plain-`DeclRef` unresolved-external direct scalar-call
route native signature and result authority before LIR verification or any
idea-734 receiver work relies on that call.

## Why This Exists

The Step 7.32 probe for a direct, fixed, nonvariadic, zero-argument external
`double` call found that production records link-ID declaration facts in
`extern_decl_link_name_map` and `extern_decls`, but does not retain a
`FnPtrSig` for a plain `DeclRef`. Therefore the route has neither a structured
fixed-empty signature nor a `fresh_value(ctx)`-allocated call-result
`LirValueId`. A verifier-only packet cannot establish facts the producer never
publishes.

## In Scope

- trace the source-level producer path for unresolved direct external scalar
  calls originating from plain `DeclRef`
- retain or construct the native `FnPtrSig` at the authoritative producer seam
  and use it to publish the structured fixed-empty signature where the source
  declaration actually supplies that fact
- allocate the scalar call result through the owning function's
  `fresh_value(ctx)` path before rendering, and propagate the same
  `LirValueId` through the ordinary call result path
- bind a focused unresolved-external fixed-empty scalar-call probe to the
  producer, signature, result, and reachable verifier obligations
- prove neighboring malformed cases reject: absent/invalid/mismatched external
  declaration link identity, missing or conflicting signature/return facts,
  and missing/invalid/duplicate/cross-owner result identity
- hand off only an exact native-authority contract that can unblock a later
  idea-744 Step 7.32 verifier packet

## Out Of Scope

- verifier-only acceptance of text-only unresolved external calls
- recovery from callee names, result spellings, rendered operands, type text,
  LLVM/printer output, declaration order, testcase paths, or any other text
- moving local prototypes into external-declaration rows or mutating HIR
  authority to fabricate the positive route
- call arguments, indirect calls, variadic calls, ABI-expanded calls, other
  external call shapes, conversions, floating-operation expansion, or ABI
  lowering
- new-BIR containers, receivers, importers, or any idea-734 implementation
- activation of this idea as part of recording it, or closure/supersession of
  idea 744

## Acceptance Criteria

- A plain-`DeclRef` unresolved external direct scalar call has a source-level,
  native `FnPtrSig` retained at the producer seam; a fixed zero-argument
  declaration yields a structured fixed-empty signature without text recovery.
- The producer allocates the actual scalar result with `fresh_value(ctx)` and
  all later ordinary uses receive that exact owned `LirValueId`.
- Focused positive and malformed-neighbor coverage proves declaration-link,
  signature, return-type, ownership, uniqueness, and result/type conflicts
  through reachable LIR verification.
- The handoff names the exact source fields and guarantees available to the
  later idea-744 verifier packet, with every unsupported external shape kept
  fail-closed.
- No verifier relaxation or idea-734 receiver claim is accepted until the
  source-level production facts above are proven.

## Durable Blocker And Retired Runbook Rationale

The completed research delivery in
`docs/frontend_unresolved_external_direct_call_representation/` (idea 747,
commit `df18f32e8`) establishes, across the supported source-language and
frontend/HIR surface, that no legal production-facing plain direct fixed-empty
scalar-call carrier combines native global/link identity with an absent
`target_fn`. Retained declarations necessarily resolve `target_fn`; no-target
forms lack the required native direct identity and fixed-empty declaration
facts; function-pointer forms are indirect.

This ruled out the then-current idea-746 runbook without authorizing a
fabricated route. At that point the runbook was retired, idea 746 stayed open
pending a source/frontend representation decision, and idea 744 remained
blocked. The corrected disposition below supersedes that temporary state.

## Closure Disposition

Close accepted under the user's corrected architecture. The retired route's
requirement that a legal direct external call remain a plain unresolved call
with absent `target_fn` was stale and is explicitly superseded; that disproven
route is not claimed as implemented.

A block-scope extern function declaration instead creates a normal bodyless
extern HIR `Function` in the compile module's ordinary function list. It needs
neither a separate extern-function list nor frontend reconstruction of call
facts. Later processing uses the normal direct-function path.

Commit `0d3781e70` (`Retain block-scope extern function declarations`) delivers
the durable authority intent through that corrected route:

- the parser retains the explicit block-scope extern prototype
- HIR registers a bodyless extern `Function`
- the LIR declaration and direct call share `LinkNameId` and a structured
  fixed-void signature
- the scalar result owns a `LirValueId` that reaches the following `FAdd`

Accepted supervisor proof consists of a fresh default build, 8/8 related
parser/HIR/LIR/BIR tests, and a matching `frontend_lir_call_type_ref`
regression guard of 1/1 before and 1/1 after with no new failures. The remaining
ordinary producer/verifier/matrix work returns to open idea 744 at Step 7.32.

## Reviewer Reject Signals

- Reject parsing or matching external names, `%t*` result spelling, formatted
  arguments, type text, printer/LLVM output, declaration order, or testcase
  identity to invent a `FnPtrSig`, signature, or result ID.
- Reject a verifier-only change that accepts a plain `DeclRef` call while the
  producer still lacks native fixed-empty signature and `fresh_value(ctx)`
  result authority.
- Reject moving a local prototype into `extern_decls`, mutating HIR facts, or
  adding a parallel symbol/value table solely to make the probe pass.
- Reject named-case branches, expectation downgrades, supported-to-unsupported
  changes, or display parity claimed as source-level capability progress.
- Reject broad call lowering, ABI, variadic, indirect-call, new-BIR, or
  receiver rewrites beyond the bounded unresolved-external scalar-call seam.
- Reject a handoff that omits malformed declaration/signature/result authority
  rejection or leaves the former text-only path behind a renamed abstraction.
