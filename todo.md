Status: Active
Source Idea Path: ideas/open/aarch64-codegen-header-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consolidate Dispatch Headers

# Current Packet

## Just Finished

Step 1 from `plan.md` is complete: calls-family shard declarations were folded
into `src/backend/mir/aarch64/codegen/calls.hpp`, `calls*.cpp` now include the
family header, and the empty `calls_*.hpp` shard headers were deleted. No
calls shard header survivors remain.

## Suggested Next

Execute Step 2 from `plan.md`: consolidate `dispatch_*.hpp` declarations into
`dispatch.hpp`, update dispatch-family include users, delete empty dispatch
shard headers, and prove with a fresh build plus supervisor-selected backend
tests.

## Watchouts

- Do not merge `.cpp` files in this plan.
- Do not move responsibilities between BIR, prealloc, MIR, and AArch64 codegen.
- Keep declarations grouped by old shard subtopic inside the family header.
- The calls consolidation had to give the formerly generic calls-effects helper
  declarations calls-specific names to avoid collisions in other AArch64
  implementation files that include `calls.hpp`.
- Do not start the CPP-family consolidation idea.

## Proof

Ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: passed, including 149/149 backend tests. Proof log:
`test_after.log`.
