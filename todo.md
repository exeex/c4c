Status: Active
Source Idea Path: ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Entry-Formal Publication Into Prologue Owner

# Current Packet

## Just Finished

Step 3 - Fold Entry-Formal Publication Into Prologue Owner completed the
mechanical prologue ownership packet:

- Moved `lower_entry_formal_publications` and the implementation-private
  entry-formal helpers formerly in `prologue_entry_formals.cpp` into
  `prologue.cpp`.
- Preserved the existing `lower_entry_formal_publications` declaration in
  `prologue.hpp` for `dispatch.cpp`.
- Removed `prologue_entry_formals.cpp` from `src/backend/CMakeLists.txt`.
- Deleted obsolete `prologue_entry_formals.cpp`.
- Kept ABI entry formal layout, byval entry-copy semantics, F128 carrier
  handling, shared formal preparation, diagnostics, and test expectations
  unchanged.

## Suggested Next

Next mechanical packet: clean stale ownership references and validate the
branch/prologue fold-back route. Search live source/build/test paths for
`comparison_branch_fusion` and `prologue_entry_formals`, update only stale
owner wording if any remains, and keep historical/source-intent references out
of scope unless lifecycle explicitly owns them.

## Watchouts

Keep this route mechanical. Do not change prepared branch-condition authority,
condition-code policy, branch target selection, ABI entry formal layout, byval
entry-copy semantics, F128 carrier handling, diagnostics, or tests. The next
packet should be cleanup/validation only unless a stale live reference requires
a narrow wording/include fix.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_function_traversal|backend_prealloc_formal_publications)$' | tee test_after.log`

`test_after.log` is the proof log.
