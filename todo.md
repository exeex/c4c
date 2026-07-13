Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 5.2
Current Step Title: Close active-source string-authority guard debt

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

## Suggested Next

- Begin Step 6 packet 1 only: migrate `globals.cpp` and
  `global_initializers.cpp`, adding the smallest module IDs, values,
  attributes, builders, and verifier rules needed for that semantic family.

## Watchouts

- Step 6 must migrate globals atomically through the new builders and verifier;
  scalar and aggregate forms stay on explicit rejection paths until their own
  ordered packets.
- Do not restore legacy importer translation units or use archived declarations
  as active implementation authority.
- Do not introduce prepared/target authority or jump to the BIR-to-MIR shell.

## Proof

- The supervisor-selected Step 5.2 proof completed green: `cmake --preset
  default && cmake --build --preset default -j 2 && python3
  scripts/test_string_authority_guard.py && ctest --test-dir build
  --output-on-failure -R '^(string_authority_guard|backend_lir_to_bir_interface)$'
  && ctest --test-dir build -j 2 --output-on-failure`.
- Guard self-tests passed 5/5, the focused CTest subset passed 2/2, and broader
  CTest passed 3030/3030.  Canonical proof output is `test_after.log`.
- `git diff --check` passed.
