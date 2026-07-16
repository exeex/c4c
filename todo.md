# Current Packet

Status: Active
Source Idea Path: ideas/open/851_hir_function_signature_definition_provenance_architecture_blocker.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm the production provenance timeline

## Just Finished

- 849 Step 2 was intentionally concluded no-change before edits. Production
  `lower_function` callers have no module-issued aggregate fact; no code,
  tests, todo update, or post-change proof resulted.

## Suggested Next

- Execute 851 Step 1: verify the caller/registration timeline and determine
  whether any earlier legal definition-provenance owner exists.

## Watchouts

- Do not retry 848 Step 2b unless a separately scoped successor supplies a
  direct fact before signature normalization.
- Do not use parser/`TypeSpec`/record/tag/owner/text recovery, a `Node*` map,
  test-only injection, `qtype_from` attachment, or LIR work.
- Preserve accepted evidence: 849 Step 1 `109ea13f4`; 848 Step 2a
  `359a9b94b`; 850 lifecycle conclusion `1723df997` with its fresh-build and
  `frontend_hir_tests` baseline.

## Proof

- Lifecycle/architecture transition only. No new build or test claim is made;
  851 may use read-only traces to make its decision.
