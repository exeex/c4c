Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 6.1
Current Step Title: Globals and global initializers

# Current Packet

## Just Finished

- Step 5.2 is complete: the active string-authority scanner now excludes only
  quarantined `src/backend/legacy/**`, with self-tests proving that the legacy
  subtree is skipped while active backend siblings remain scanned.
- The sole new active-BIR classification is
  `ModuleData::functions_by_link_name_`: `FunctionId` remains entity identity,
  the map is only the ABI-spelling uniqueness/declaration-definition merge
  index, FoundationVerifier proves both directions exactly, and removal waits
  for an owner-correct interned link identity.
- Step 5 is complete through `2b6148590`; its selected full proof passed CTest
  3030/3030.

## Suggested Next

- Execute Step 6.1 only: reimplement module globals and the explicitly
  supported global-initializer subset through new-BIR IDs, storage/order,
  builders, views, and verifier rules.
- Add direct retained-interface proof for a global declaration, an accepted
  initializer, and an unsupported initializer that rejects observably.

## Watchouts

- `src/backend/bir/lir_to_bir/globals.cpp` and
  `global_initializers.cpp` are behavior inventory, not implementation to
  reactivate unchanged or a source of legacy BIR authority.
- Do not migrate `scalar.cpp` or `aggregate.cpp` in this packet.  A constant
  representation owned solely by global initializers is not permission to add
  function-body scalar or aggregate instructions.
- Do not restore legacy importer translation units, transplant archived BIR,
  or introduce prepared, target, prealloc, or BIR-to-MIR authority.
- Unsupported initializer forms must return structured failure; never report
  success with empty, zeroed, or dropped semantics.

## Proof

- The supervisor-selected Step 5.2 proof completed green: `cmake --preset
  default && cmake --build --preset default -j 2 && python3
  scripts/test_string_authority_guard.py && ctest --test-dir build
  --output-on-failure -R '^(string_authority_guard|backend_lir_to_bir_interface)$'
  && ctest --test-dir build -j 2 --output-on-failure`.
- Guard self-tests passed 5/5, the focused CTest subset passed 2/2, and broader
  CTest passed 3030/3030.  Canonical proof output is `test_after.log`.
- `git diff --check` passed.
