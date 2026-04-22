# Execution State

Status: Active
Source Idea Path: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Classify And Repair The Newly Exposed Post-`ret()` Runtime Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Plan step `2.3` classified the newly exposed `stdarg()` fault as still
idea-72-owned aggregate-call/runtime handoff work, not idea 71 variadic
traversal. A prepared-BIR probe for `stdarg()` shows the first failing call is
still `bir.call void myprintf(ptr @.str49, i64 %t6, i64 %t9, i64 %t13, i64 %t16, ptr @s9, ptr @s9, ptr @s9, ptr @s9)`,
and the first bad loads come from aggregate-pack source pointers `%t4`,
`%t7`, `%t11`, and `%t14` that feed `load_local ... addr %tN` before
`myprintf` starts consuming `va_list`. I patched the generic pointer-memory
path in `prepared_local_slot_render.cpp` to prefer authoritative stack-address
rematerialization and updated
`find_prepared_authoritative_named_stack_offset_if_supported()` in
`x86_codegen.hpp` to fall back to prepared stack-home `offset_bytes` when a
canonical frame-slot map entry is unavailable. Fresh focused proof now shows
the four stale aggregate-pack loads in `stdarg()` changed from
`mov r10, QWORD PTR [rsp + home]` to `lea r10, [rsp + home]`, clearing the old
null-pointer dereference before `myprintf` begins variadic traversal. The
owned case advances beyond `stdarg()` into a later runtime fault in `match()`
at `movsbl (%rax), %eax`, where `%rax == 0x000000b000000030`.

## Suggested Next

Stay on plan step `2.3` and classify the new `match()` crash to decide whether
the first bad `%rax == 0x000000b000000030` pointer still belongs to aggregate
call/runtime handoff ownership or has now graduated into a later string or
variadic-runtime leaf. The next packet should trace the `match()` arguments
back to the first corrupted pointer-producing call/result handoff and either
repair that idea-72-owned seam or record an explicit rehome boundary out of
idea 72 if the first bad fact now belongs elsewhere.

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
- The repaired `stdarg()` seam was pointer-value materialization for the first
  `%9s` aggregate-pack copy sequence, not later `va_arg` traversal.
- `--dump-prepared-bir` shows the owned failing loads as `%t5.memcpy.copy.0 =
  bir.load_local i32 %t5, addr %t4`, `%t8.memcpy.copy.0 = bir.load_local i8
  %t8, addr %t7`, `%t12.memcpy.copy.0 = bir.load_local i32 %t12, addr %t11`,
  and `%t15.memcpy.copy.0 = bir.load_local i8 %t15, addr %t14`, with homes
  `%t4 -> offset 10616`, `%t7 -> 11320`, `%t11 -> 10624`, `%t14 -> 11328`.
- The generated `00204.c.s` now emits `lea r10, [rsp + 10616]`,
  `[rsp + 11320]`, `[rsp + 10624]`, and `[rsp + 11328]` at the old crash site,
  proving the aggregate-pack sources now rematerialize stack addresses instead
  of loading uninitialized pointer-home contents.
- The new first bad stop is downstream in `match()`, not `stdarg()`. Do not
  assume that later crash automatically belongs to idea 71; verify the first
  corrupted pointer/result handoff before rehoming.

## Proof

Focused runtime probe for step-2.3 aggregate-pack ownership classification:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result: `backend_x86_handoff_boundary` PASS; `c_testsuite_x86_backend_src_00204_c`
still fails with `[RUNTIME_NONZERO] ... exit=Segmentation fault`.

Crash-surface confirmation:
`./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c > /tmp/00204.prepared_bir.txt`
`gdb -batch -ex 'run' -ex 'bt' -ex 'frame 0' -ex 'x/24i $rip-24' -ex 'info reg rsp rbp rip rdi rsi rdx rcx r8 r9 r10 r11 rax al' --args build/c_testsuite_x86_backend/src/00204.c.bin`
and `build/c_testsuite_x86_backend/src/00204.c.s`
Result: the old `stdarg()` stop is gone. Rebuilt asm now shows
`lea r10, [rsp + 10616]`, `[rsp + 11320]`, `[rsp + 10624]`, and
`[rsp + 11328]` before the first `myprintf` call, proving the aggregate-pack
pointer homes are rematerialized as stack addresses. The new first bad stop is
`match()` at `movsbl (%rax), %eax`, with `%rax == 0x000000b000000030`, so the
owned case has advanced into a later runtime seam. Proof log path is
`test_after.log`.
