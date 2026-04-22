# Execution State

Status: Active
Source Idea Path: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Aggregate-Call Runtime Seam
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Plan step `2.1` repaired the prepared same-module helper aggregate-parameter
materialization seam enough to move `00204.c` past the original `fa_s2`
crash: the generic bounded helper renderer now refreshes pointer/byval param
homes from their incoming ABI registers at the active prepared pointer-access
sites, so helper callees consume the forwarded aggregate address instead of
dereferencing stale stack junk. The focused probe no longer crashes at
`fa_s2`; it now advances to `fa_hfa11` and dies inside `__printf`, which
narrows the next live runtime seam to helper-side floating/HFA variadic-call
ABI handling rather than fixed-arity aggregate address materialization.

## Suggested Next

Take the next packet on the advanced `fa_hfa11` crash surface: inspect the
prepared same-module helper floating/HFA path around the variadic `printf`
call, and repair the helper-side ABI/alignment/materialization contract that
now fails after the fixed-arity aggregate-byval seam is removed.

## Watchouts

- Do not reopen idea 70's resolved post-assembly link-closure seams unless a
  fresh proof shows unresolved same-module or direct variadic-runtime
  references again.
- Do not force this fixed-arity aggregate-call crash back into idea 71 unless
  the first failing runtime seam advances into genuine variadic execution.
- Treat the current crash as runtime semantics work, not a boundary-contract
  issue.
- Preserve the split between runtime ownership in idea 72 and earlier
  prepared-module or post-assembly ownership in ideas 61, 70, and 71.
- The repaired seam lives in the bounded same-module helper renderer; do not
  reopen `calls.cpp` or `prologue.cpp` unless a fresh proof shows the helper
  path has handed off back into the generic x86 call/prologue route.
- The pointer-param home for helpers like `fa_s2` is not a durable frame
  spill; overlapping byte-copy locals can clobber that home, so the current
  repair refreshes the authoritative pointer home immediately before each
  prepared pointer-based access.
- The next failing surface is no longer fixed-arity aggregate forwarding.
  The focused gdb probe now stops in `fa_hfa11` during `__printf`, which
  suggests a helper-side floating/HFA variadic ABI seam.

## Proof

Focused runtime probe for step-2.1 aggregate-helper repair:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result: `backend_x86_handoff_boundary` PASS; `c_testsuite_x86_backend_src_00204_c`
still fails with `[RUNTIME_NONZERO] ... exit=Segmentation fault`, but the
live crash surface advanced beyond `fa_s2`.

Crash-surface confirmation for the repaired seam:
`gdb -batch -ex 'run' -ex 'bt' -ex 'info registers rdi r10 rsp' -ex 'x/16i $pc-16' --args build/c_testsuite_x86_backend/src/00204.c.bin`
and `build/c_testsuite_x86_backend/src/00204.c.s`
Result: generated `fa_s2` now refreshes `QWORD PTR [rsp]` from `%rdi` before
each pointer-backed byte load and no longer crashes there; the new stop is
inside `__printf` called from `fa_hfa11`. Proof log path is `test_after.log`.
