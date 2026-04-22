# Execution State

Status: Active
Source Idea Path: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Aggregate-Call Runtime Seam
Plan Review Counter: 3 / 4
# Current Packet

## Just Finished

Plan step `2.1` repaired the selected small-aggregate return-helper seam:
bounded same-module x86 return helpers now home `%ret.sret` into stack-slot
pointer homes and refresh that home before pointer-based copyout writes, so
`fr_s1`/`fr_s2` no longer dereference an uninitialized destination pointer
slot after the long-double HFA seam was cleared. The focused proof still
fails, but `00204.c` now executes beyond the `fr_s1` helper family and
graduates into a later variadic-runtime crash inside `ret()`.

## Suggested Next

Take the next packet on the advanced post-`fr_s1` crash surface: inspect the
`ret()` variadic `printf("%.1f\\n", ...)` call path after the aggregate-return
helpers and repair the remaining x86-64 stack-alignment/accounting seam that
still reaches `__printf` with `%rsp` misaligned for SSE register saveout.

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
- The repaired small-aggregate return-helper seam lives in the same bounded
  helper-prefix renderer: `TypeKind::Ptr` stack-slot homes must materialize the
  incoming register, and `%ret.sret` must participate in the pointer-home
  refresh path just like other pointer params.
- The next failing surface is no longer `fr_s1`; the focused gdb probe now
  advances into `ret()` and stops in glibc `__printf("%.1f\\n")`, where
  `movaps %xmm0, 0x50(%rsp)` faults with `al=1`, indicating the caller still
  reaches that variadic site with misaligned stack state.

## Proof

Focused runtime probe for step-2.1 aggregate/helper runtime repair:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result: `backend_x86_handoff_boundary` PASS; `c_testsuite_x86_backend_src_00204_c`
still fails with `[RUNTIME_NONZERO] ... exit=Segmentation fault`, but the
live crash surface advanced beyond the `fr_s1`/`fr_s2` return-helper leaf into
the later `ret()` variadic-runtime path.

Crash-surface confirmation for the repaired seam:
`gdb -batch -ex 'run' -ex 'bt' -ex 'frame 5' -ex 'x/24i $pc-24' -ex 'info registers rsp rbp rdi rsi rdx rcx r8 r9 eax al' --args build/c_testsuite_x86_backend/src/00204.c.bin`
and `build/c_testsuite_x86_backend/src/00204.c.s`
Result: generated `fr_s1`/`fr_s2` now emit the missing `mov QWORD PTR [rsp],
rdi` home store before helper copyout writes and no longer crash there; the
new stop is in glibc `__printf` on the `ret()` path, where `movaps` faults
while saving `%xmm0`, indicating a later variadic-call alignment/runtime seam.
Proof log path is `test_after.log`.
