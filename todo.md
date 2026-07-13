# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Complete foundational identities, types and module containers

## Just Finished

- Plan Step 2 packet 2B completed: added epoch-owned typed Raw IDs and ordered,
  immutable module views for the current LIR `link_names`, `struct_names`, and
  `struct_decls` surfaces, preserving source IDs, spellings, declaration order,
  packed/opaque facts, and structured field types.
- ModuleBuilder owns the staged tables, declaration storage, and lookup caches;
  importer validation and the publication verifier reject malformed names,
  unresolved fields, duplicate/conflicting declarations, and cache/index drift
  transactionally. Legacy `type_decls` text is not used as declaration authority.

## Suggested Next

- Execute Step 2C only: add the next bounded foundational constant/value and
  forward-use identity packet without entering globals, signatures, or CFG.

## Watchouts

- Step 2A intentionally retains the existing zero-parameter, void-definition
  importer boundary. Non-void `TypeBase` receipt, complete signatures, and CFG
  receipt remain Step 4A and are not claimed by this packet.
- Raw struct declarations now accept named forward/shared references only when
  their source `StructNameId` and spelling resolve through the imported table;
  preserve that reference-domain rule in later value families.
- `type_decls` remains a compatibility shadow and must not regain semantic
  authority. Globals, externs, intrinsics, specializations, complete signatures,
  CFG, and Step 2C constant/forward-use work remain outside packet 2B.

## Proof

- Passed the supervisor-selected exact broader matching regression checkpoint:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|frontend_lir_)'`.
- The fresh post-change run passed 8/8 tests. Canonical proof log:
  `test_after.log`.
