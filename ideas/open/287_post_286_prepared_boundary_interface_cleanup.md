# Post-286 Prepared Boundary Interface Cleanup

## Goal

Turn the interface design problems exposed by the completed AArch64 00204
stdarg repair into a bounded prepared-boundary cleanup plan.

The cleanup should reduce string-carrier and policy-coupling debt without
reopening the completed 286 capability work or broadening into a general
AArch64 ABI rewrite.

## Why This Exists

Idea 286 closed successfully, but the closure path exposed three interface
smells that are broader than the named stdarg fixture:

- call argument type and ABI suffix data still cross the BIR lowering boundary
  as rendered strings, requiring helpers such as `strip_call_arg_abi_type_suffix`
  for `alignstack(...)`;
- semantic call lowering now directly expands AArch64 variadic HFA
  carrier-array arguments into FP lanes, which couples generic call lowering,
  target ABI policy, and local aggregate slot aliases in one place;
- opaque pointer provenance admits byte offsets through a coarse
  `allow_opaque_ptr_base && stored_type == I8` rule instead of a structured
  base-object extent / byte-range carrier.

This overlaps with the earlier prepared cleanup direction: retained prepared
policy surfaces should become clearer, narrower, and more explicit rather than
being migrated into BIR by string parsing or target-specific side channels.

## In Scope

- Audit the post-286 call argument interface between LIR, semantic BIR, and
  prepared/prealloc for:
  - structured type identity;
  - ABI suffixes such as `alignstack`;
  - byval/carrier array layout;
  - variadic HFA lane metadata.
- Decide whether `LirCallArg`, `LirTypeRef`, or an adjacent call-argument
  metadata carrier should own ABI suffix facts instead of rendered text.
- Identify the smallest cleanup packet that can remove or quarantine
  `strip_call_arg_abi_type_suffix` from semantic lowering logic.
- Audit whether AArch64 variadic HFA carrier-array lane expansion should stay
  in semantic call lowering or move behind a narrower target ABI helper with a
  structured input/output contract.
- Audit the opaque pointer byte-offset admission rule and decide whether a
  structured extent/range carrier is needed before further prepared
  `memory_accesses` cleanup.
- Produce follow-up ideas if the call-argument cleanup, HFA helper boundary, or
  opaque-pointer range carrier must be implemented separately.

## Out of Scope

- Reopening 286's already-closed AArch64 00204 semantic BIR admission work.
- Rewriting AArch64 call ABI classification, variadic entry planning, or
  prepared call plans wholesale.
- Migrating retained prepared/prealloc policy surfaces into BIR just because
  semantic lowering now has a passing fixture.
- Deleting public prepared lookup surfaces without a matching route/BIR
  agreement and fail-closed proof.
- Expectation-only cleanup or helper renames that leave the same string-carrier
  or target-policy coupling in place.

## Cleanup Plan

1. Inventory the current post-286 carriers and bridges:
   `LirCallArg`, `LirTypeRef`, call arg rendered strings, aggregate aliases,
   local aggregate slots, `CallArgAbi`, prepared call plans, and opaque pointer
   provenance.
2. Classify each exposed smell as one of:
   - safe local cleanup now;
   - needs a structured carrier before implementation;
   - retained prepared/target policy that should be documented, not migrated;
   - separate follow-up idea.
3. For call argument type/ABI suffixes, draft the preferred ownership boundary:
   which layer stores `alignstack`, byval/carrier shape, and struct identity,
   and which layer may consume rendered text only as legacy compatibility.
4. For AArch64 variadic HFA carrier arrays, decide whether semantic BIR should
   call a target ABI helper that returns lane values/ABI records rather than
   assembling those records inline.
5. For opaque pointer byte offsets, decide whether an explicit extent/range
   carrier is required before prepared `memory_accesses` demotion can continue
   safely.
6. Implement only one narrow cleanup packet if the audit finds a self-contained
   change; otherwise close this idea with generated implementation follow-ups.

## Acceptance Criteria

- The closure note contains a concrete boundary map for the three exposed
  surfaces: call arg type/ABI suffixes, AArch64 variadic HFA lane expansion,
  and opaque pointer byte-offset provenance.
- If code changes are made, they remove or quarantine one real string-carrier
  or policy-coupling path and include focused proof that 286's stdarg tests
  still pass.
- If no code change is safe yet, the closure generates follow-up ideas with
  clear ownership, files, proof commands, and reject signals.
- The existing 286 proof subset remains green:
  `backend_lir_to_bir_notes`,
  `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`, and
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.
- Any prepared cleanup claim preserves prepared-only, unsupported, mismatch,
  and target-policy fail-closed behavior.

## Reviewer Reject Signals

- The route claims prepared cleanup while only renaming helpers, moving code, or
  updating expectations.
- The patch broadens AArch64 ABI policy or call-plan behavior without a
  structured interface decision and focused proof.
- The patch deletes rendered-string compatibility while leaving no structured
  `LirTypeRef` / `LirCallArg` / ABI carrier for the same fact.
- The patch treats the passing 00204 fixture as evidence that public prepared
  call-plan or memory-access surfaces can be retired wholesale.
- The patch moves target/prepared policy into semantic BIR without preserving
  fail-closed behavior for unsupported, mismatch, prepared-only, or
  policy-sensitive cases.
- The exact old interface smell remains but is hidden behind a differently
  named helper.
