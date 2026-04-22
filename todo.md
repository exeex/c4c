# Execution State

Status: Active
Source Idea Path: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Aggregate-Call Runtime Seam
Plan Review Counter: 2 / 4
# Current Packet

## Just Finished

Plan step `2.1` repaired the next helper-side long-double/HFA runtime seam:
prepared direct-extern helper calls with a nonzero local frame now keep the
same x86-64 call-alignment fix even when the call also materializes stacked
arguments, so long-double HFA helpers like `fa_hfa31`/`fa_hfa32` no longer
enter variadic libc calls with the stack 8 bytes off. The focused proof still
fails, but `00204.c` now executes beyond the `fa_hfa31` family and graduates
into the later return-helper leaf `fr_s1`.

## Suggested Next

Take the next packet on the advanced `fr_s1` crash surface: inspect the
prepared return-helper path for small aggregate returns and repair the helper
result-pointer home or writeback contract that now fails after the long-double
HFA variadic-call alignment seam is cleared.

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
- The repaired float/double HFA seam is specifically the outbound direct-extern
  call alignment path for prepared helpers with an existing local frame; keep
  the fix scoped there unless later proof shows stack-arg-bearing helpers need
  a broader variant.
- The next failing surface is no longer `fa_hfa11`; the focused gdb probe now
  advances through the long-double HFA helper family and stops later in
  `fr_s1`, where the small-aggregate return helper writes through an invalid
  destination pointer home.

## Proof

Focused runtime probe for step-2.1 aggregate/helper runtime repair:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result: `backend_x86_handoff_boundary` PASS; `c_testsuite_x86_backend_src_00204_c`
still fails with `[RUNTIME_NONZERO] ... exit=Segmentation fault`, but the
live crash surface advanced beyond the `fa_hfa31` long-double HFA leaf into a
later return-helper leaf.

Crash-surface confirmation for the repaired seam:
`gdb -batch -ex 'run' -ex 'bt' -ex 'frame 5' -ex 'x/24i $pc-24' -ex 'info registers rsp rbp rdi rsi rdx rcx r8 r9 eax al' --args build/c_testsuite_x86_backend/src/00204.c.bin`
and `build/c_testsuite_x86_backend/src/00204.c.s`
Result: generated `fa_hfa31`/`fa_hfa32` now emit the missing `sub rsp, 8` /
`add rsp, 8` around their variadic `printf` calls and no longer crash there;
the new stop is in `fr_s1`, where the helper writes through a bad destination
pointer. Proof log path is `test_after.log`.
