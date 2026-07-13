# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Complete foundational identities, types and module containers

## Just Finished

- Plan Step 2 packet 2A completed: added closed typed Raw receipt for every
  current `LirTypeKind`, including arbitrary integer widths, floating and VRM
  widths, exact composite spellings, and `StructNameId` identity.
- The bounded void-signature receipt now preserves structured `TypeSpec` base,
  pointer/reference/array/function-pointer shape facts and reconciles them with
  the optional `LirTypeRef` mirror; inline-asm bindings reuse the same mirror
  conversion. Builders/views/verifier preserve and check these facts, and
  malformed staged Raw types cannot publish.

## Suggested Next

- Execute Step 2B only: add module type/name tables and stable module IDs for
  `struct_decls`, `link_names`, and `struct_names`, with deterministic order,
  immutable views, builder ownership, and cache-parity verification.

## Watchouts

- Step 2A intentionally retains the existing zero-parameter, void-definition
  importer boundary. Non-void `TypeBase` receipt, complete signatures, and CFG
  receipt remain Step 4A and are not claimed by this packet.
- Named struct mirrors require a resolving, spelling-matching `StructNameId`,
  while actual named definitions and conflict/cache checks remain Step 2B.
- Module type declarations are still rejected transactionally; do not parse
  their rendered text or move Step 2C constants/forward-use work into Step 2B.

## Proof

- Passed the supervisor’s final broader matching regression checkpoint:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|frontend_lir_)'`.
- The matching baseline and post-change runs both passed 8/8 tests. Canonical
  logs: `test_before.log` and `test_after.log`.
