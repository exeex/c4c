# Execution State

Status: Active
Source Idea Path: ideas/open/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Permanent-Home Agreement Runtime Seam
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Completed a bounded step-`2` repair by narrowing the fixed permanent-home
preference inside
`find_prepared_authoritative_named_stack_offset_if_supported(...)` to
address-exposed `local_slot` objects only. Fresh proof confirms the helper-lane
regression is gone while the `%lv.s` repair remains effective: generated
`myprintf` still stores the live cursor with `mov QWORD PTR [rsp], rax` and
passes `match(&s, ...)` as `lea rdi, [rsp]`, so
`c_testsuite_x86_backend_src_00204_c` stays beyond the old `match()`-entry
segmentation fault and now fails later as `[RUNTIME_MISMATCH]`. The delegated
guard subset now matches the packet goal because
`backend_x86_handoff_boundary` passes again.

## Suggested Next

Inspect the new later `00204.c` runtime-mismatch seam and verify whether idea
73 still owns it. Start from the post-`match()` state now that `%lv.s` uses
its authoritative home, identify the first corrupted argument or return family,
and either continue with the smallest local-home packet inside idea 73 or hand
the seam back for supervisor rehome if ownership has moved downstream.

## Watchouts

- Do not reopen idea 72 unless fresh proof moves the first bad fact back into
  aggregate-by-value argument or result transport.
- Do not force this seam into idea 71 unless execution advances into genuine
  `llvm.va_start`, `va_list`, or later variadic traversal ownership.
- Do not reopen idea 68's pre-codegen local-slot handoff family unless the
  current emitted-home mismatch first regresses back into an x86 rejection.
- Keep the fix at the shared local-home ownership layer; helper-name or
  testcase-name aliases for `myprintf`, `match`, or `00204.c` are route drift.
- Preserve the factual advancement: `myprintf` now passes `match(&s, ...)`
  from `[rsp]`, so the first bad fact is no longer the old `%lv.s` crash seam.
- The narrowing depends on `source_kind == "local_slot"` plus
  `address_exposed`; widening the override back to params or other fixed homes
  reopens the helper-lane regression.
- The proof now isolates `00204.c` to a later wrong-result/runtime family, so
  the next packet should verify ownership before assuming more local-home work.

## Proof

Step `2` proof run completed with the delegated scope:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  after staying beyond the old `match()` crash
- Proof log path: `test_after.log`
Supplemental evidence:
inspect `build/c_testsuite_x86_backend/src/00204.c.s` around `myprintf` for
`mov QWORD PTR [rsp], rax` followed by `lea rdi, [rsp]` before `call match`,
and rerun `backend_x86_handoff_boundary` to confirm the byval-i8 helper
pointer-arg lane stays green.
