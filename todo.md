# Execution State

Status: Active
Source Idea Path: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Aggregate-Call Runtime Seam
Plan Review Counter: 4 / 4
# Current Packet

## Just Finished

Plan step `2.1` repaired the next aggregate-call runtime handoff after the
`fr_s1` destination-pointer fix: bounded same-module x86 prepared calls now
apply the missing 8-byte outgoing stack-alignment pad on both the general
same-module defined-call lane and the helper-call fast path, so zero-stack
calls such as `pcs -> ret`, `ret -> fr_s1`, and `ret()`'s variadic
`printf("%.1f\\n", ...)` no longer enter callees with the wrong SysV x86-64
stack parity. The focused proof still fails, but `00204.c` now executes past
the glibc `__printf` crash in `ret()` and advances into a later runtime fault
inside `stdarg()`.

## Suggested Next

Take the next packet on the advanced post-`ret()` crash surface: inspect the
`stdarg()` runtime path, especially the prepared variadic aggregate/home loads
around the faulting `mov (%r10), %eax` fed from `[rsp + 0x2978]`, and repair
the remaining x86-64 variadic-runtime pointer/accounting seam now exposed
after the call-alignment handoff was corrected.

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
- The repaired handoff lives in `prepared_local_slot_render.cpp`: both the
  bounded same-module helper-call fast path and the general same-module
  defined-call path now emit the missing `sub rsp, 8` / `add rsp, 8` around
  zero-stack outgoing calls when the prepared frame parity requires it.
- The next failing surface is no longer `ret()`'s variadic `printf`; the
  focused gdb probe now stops in `stdarg()` at `mov (%r10), %eax`, where
  `%r10` comes from `[rsp + 0x2978]` and is invalid, indicating the next seam
  is variadic aggregate/runtime home tracking rather than stack-call parity.

## Proof

Focused runtime probe for step-2.1 aggregate/helper runtime repair:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result: `backend_x86_handoff_boundary` PASS; `c_testsuite_x86_backend_src_00204_c`
still fails with `[RUNTIME_NONZERO] ... exit=Segmentation fault`, but the
live crash surface advanced beyond the `ret()` variadic `printf` seam into the
later `stdarg()` runtime path.

Crash-surface confirmation for the repaired seam:
`gdb -batch -ex 'run' -ex 'bt' -ex 'frame 0' -ex 'x/24i $rip-24' -ex 'info reg rsp rbp rip rdi rsi rdx rcx r8 r9 rax al' --args build/c_testsuite_x86_backend/src/00204.c.bin`
and `build/c_testsuite_x86_backend/src/00204.c.s`
Result: `pcs` now emits the missing aligned same-module call shims before
`ret` and `stdarg`, and `ret` now emits the same alignment shim before helper
and variadic `printf` calls. The previous glibc `__printf("%.1f\\n")` `movaps`
fault is gone; the new stop is in `stdarg()` at `mov (%r10), %eax`, after
loading `%r10` from `[rsp + 0x2978]`. Proof log path is `test_after.log`.
