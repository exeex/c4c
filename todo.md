# Execution State

Status: Active
Source Idea Path: ideas/open/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Address-Exposed Local-Home Crash Surface
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Completed step `1` owner classification for idea 73. The authoritative
permanent home for `myprintf` `%lv.s` is owned by prepared stack-layout object
and slot assignment, not by aggregate-call transport: prepared dump evidence
shows `object #1666 func=myprintf name=%lv.s ... address_exposed=yes
requires_home_slot=yes permanent_home_slot=yes` and
`slot #1637 object_id=1666 func=myprintf offset=0 ... fixed_location=yes`,
while emitted asm still stores the live cursor with `mov QWORD PTR [rsp], rax`
but passes `match(&s, ...)` as `lea rdi, [rsp + 80]`. The smallest shared
idea-73 seam is the x86 authoritative named-stack-offset resolver:
`find_prepared_authoritative_named_stack_offset_if_supported(...)` currently
returns `local_layout.offsets[value_name]` before consulting fixed prepared
frame-slot/home-slot identity, and the same-module helper-call path
`append_prepared_named_ptr_argument_move_into_register_if_supported(...)`
consumes that stale offset for by-reference pointer arguments like `%lv.s`.

## Suggested Next

Execute the first repair packet for idea-73 step `2`: change the shared
authoritative named-stack-offset resolution path so address-exposed permanent
home locals prefer prepared frame-slot/home-slot identity before transient
`local_layout.offsets`, then recheck that `myprintf` passes `&s` from the same
authoritative home as the live `%lv.s` store.

## Watchouts

- Do not reopen idea 72 unless fresh proof moves the first bad fact back into
  aggregate-by-value argument or result transport.
- Do not force this seam into idea 71 unless execution advances into genuine
  `llvm.va_start`, `va_list`, or later variadic traversal ownership.
- Do not reopen idea 68's pre-codegen local-slot handoff family unless the
  current emitted-home mismatch first regresses back into an x86 rejection.
- Keep the fix at the shared local-home ownership layer; helper-name or
  testcase-name aliases for `myprintf`, `match`, or `00204.c` are route drift.
- Preserve non-permanent and slice-based local-slot addressing. The narrowed
  seam is specifically precedence between transient `local_layout.offsets` and
  prepared fixed home-slot identity for address-exposed permanent homes.
- The same resolver feeds multiple pointer-address render paths, so the repair
  must avoid regressing ordinary stack-local pointer operands that legitimately
  live in reorderable local slots.

## Proof

Step `1` proof run completed with the delegated scope:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_NONZERO]` and
  `exit=Segmentation fault`
- Proof log path: `test_after.log`
Supplemental owner evidence used for classification:
`./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c > /tmp/00204.prepared_bir.txt`
and inspect `build/c_testsuite_x86_backend/src/00204.c.s`.
