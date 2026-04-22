# Execution State

Status: Active
Source Idea Path: ideas/open/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Address-Exposed Local-Home Crash Surface
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Lifecycle switch on 2026-04-22 closed idea 72 after `00204.c` advanced out of
the aggregate-call runtime family. The active runbook is now idea 73, which
owns the downstream `myprintf` address-exposed local-home mismatch where
`%lv.s` is stored at `[rsp]` but `match(&s, ...)` still receives `[rsp + 80]`.

## Suggested Next

Execute step `1` of `plan.md`: confirm the exact semantic owner of `%lv.s`'s
permanent home slot in emitted `myprintf`, then narrow the first repair packet
to the shared local-home placement/refresh seam that makes `match(&s, ...)`
observe the same address as the live cursor store.

## Watchouts

- Do not reopen idea 72 unless fresh proof moves the first bad fact back into
  aggregate-by-value argument or result transport.
- Do not force this seam into idea 71 unless execution advances into genuine
  `llvm.va_start`, `va_list`, or later variadic traversal ownership.
- Do not reopen idea 68's pre-codegen local-slot handoff family unless the
  current emitted-home mismatch first regresses back into an x86 rejection.
- Keep the fix at the shared local-home ownership layer; helper-name or
  testcase-name aliases for `myprintf`, `match`, or `00204.c` are route drift.
- The current boundary evidence is specific: `mov QWORD PTR [rsp], rax` holds
  the live `%lv.s` cursor while `match(&s, ...)` still receives
  `lea rdi, [rsp + 80]`.

## Proof

No executor packet has run under idea 73 yet. Initial proof scope for step `1`:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Supplement with the existing crash-surface evidence already recorded in the
previous packet:
`./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c > /tmp/00204.prepared_bir.txt`
`gdb -batch -ex 'run' -ex 'bt' -ex 'frame 0' -ex 'x/24i $rip-24' -ex 'info reg rsp rbp rip rdi rsi rdx rcx r8 r9 r10 r11 rax al' --args build/c_testsuite_x86_backend/src/00204.c.bin`
and inspect `build/c_testsuite_x86_backend/src/00204.c.s`.
