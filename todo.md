Status: Active
Source Idea Path: ideas/open/aarch64-codegen-header-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consolidate Dispatch Headers

# Current Packet

## Just Finished

Step 2 from `plan.md` is complete: dispatch-family shard declarations were
folded into `src/backend/mir/aarch64/codegen/dispatch.hpp`, grouped by the old
shard subtopic names. Local dispatch-family include users now include
`dispatch.hpp`, empty `dispatch_*.hpp` shard headers were deleted, and no
dispatch shard header survivors remain.

## Suggested Next

Execute Step 3 from `plan.md`: review the consolidated calls and dispatch
header families, confirm no `.cpp` files were merged or renamed, confirm no
BIR/prealloc/MIR responsibility movement occurred, and run the
supervisor-selected broader validation checkpoint.

## Watchouts

- Do not merge `.cpp` files in this plan.
- Do not move responsibilities between BIR, prealloc, MIR, and AArch64 codegen.
- The dispatch consolidation exposed formerly hidden private helper name
  collisions in `dispatch_branch_fusion.cpp` and `dispatch_dynamic_stack.cpp`;
  those anonymous-namespace helpers were renamed with file-specific prefixes.
- Do not start the CPP-family consolidation idea.

## Proof

Ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: passed, including 149/149 backend tests. Proof log:
`test_after.log`.
