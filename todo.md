# Execution State

Status: Active
Source Idea Path: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Aggregate-Call Runtime Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Plan step `2.1` repaired the next helper-side floating/HFA runtime seam after
the fixed-arity aggregate-forwarding crash: prepared direct-extern helper calls
with a nonzero local frame now insert the missing 8-byte x86-64 call-alignment
pad before variadic/libc calls, so framed helpers like `fa_hfa11` and the
following float/double HFA helpers no longer enter `printf` with a misaligned
stack. The focused proof still fails, but `00204.c` now executes past
`fa_hfa11`/`fa_hfa24` and graduates into the later `fa_hfa31` long-double
helper leaf inside `__printf_fp_l_buffer`.

## Suggested Next

Take the next packet on the advanced `fa_hfa31` crash surface: inspect the
prepared helper long-double/HFA variadic path and repair the remaining
helper-side long-double argument materialization or stack contract that still
breaks once the float/double variadic call-alignment seam is fixed.

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
  advances through `fa_hfa24` and stops in `fa_hfa31` during long-double
  formatting inside `__printf_fp_l_buffer`.

## Proof

Focused runtime probe for step-2.1 aggregate/helper runtime repair:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result: `backend_x86_handoff_boundary` PASS; `c_testsuite_x86_backend_src_00204_c`
still fails with `[RUNTIME_NONZERO] ... exit=Segmentation fault`, but the
live crash surface advanced beyond `fa_hfa11` into the later long-double HFA
leaf.

Crash-surface confirmation for the repaired seam:
`gdb -batch -ex 'run' -ex 'bt' -ex 'frame 1' -ex 'info registers rdi rsi rdx rcx r8 r9 al rsp' -ex 'x/24i $pc-24' --args build/c_testsuite_x86_backend/src/00204.c.bin`
and `build/c_testsuite_x86_backend/src/00204.c.s`
Result: generated `fa_hfa11` now emits `sub rsp, 8` / `add rsp, 8` around the
variadic `printf` call and no longer crashes there; the new stop is inside
`__printf_fp_l_buffer` called from `fa_hfa31`. Proof log path is
`test_after.log`.
