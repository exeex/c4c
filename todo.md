# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Source Plan Path: plan.md
Current Step ID: 6.2
Current Step Title: Establish complete publication semantic origin at the prepared owner

## Just Finished

- Step 6.3 produced a blocked owner-only AArch64 consumer candidate that removes
  Route 5, MIR, value-home, edge-publication, and target-local routing-fact
  reconstruction in favor of the attached stable successor/value/role query.
- Exact backend proof showed that Step 6.2 is not complete: unchanged supported
  fixtures still attach otherwise positive facts with `Unknown` publication
  semantic origin, so the prepared query correctly returns `Mismatched`.

## Suggested Next

- Execute Step 6.2 at the prepared owner/publication boundary: trace the
  unchanged supported fixture publication path, establish concrete semantic
  origin from complete edge semantics, and prove incomplete publication still
  fails closed.
- Preserve the uncommitted Step 6.3 owner-only consumer as blocked candidate
  work. After Step 6.2 focused proof is green, rerun that candidate unchanged
  against the exact backend subset.

## Watchouts

- Do not restore Route 5 or target-local fallback to mask the missing prepared
  semantic origin; the owner-only consumer is correctly fail closed.
- Do not rewrite supported integration vectors. The blocker is upstream of the
  AArch64 consumer and outside this packet's owned files.
- Do not treat attachment alone as semantic authority: a supported positive
  must carry a concrete publication semantic origin derived from its complete
  prepared edge contract.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as delegated.
- Build passed, but the delegated backend subset failed at the unchanged
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_current_block_join_routing`, and
  `backend_aarch64_current_block_fixture_policy_attachment` contracts because
  owner facts with `Unknown` semantic origin are rejected. Proof log:
  `test_after.log`.
