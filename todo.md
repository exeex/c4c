# Execution State

Status: Active
Source Idea Path: ideas/open/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Byval-Param Pointer-Argument Crash Surface
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Lifecycle switch completed after closing idea 73. The accepted `%lv.s`
permanent-home repair kept `myprintf` on its authoritative local-slot home, and
fresh focused proof shows the first remaining bad fact now occurs earlier in
fixed-arity helpers such as `fa_s1`: generated asm stores incoming byval
pointer `%p.a` at `[rsp]` but still calls `printf("%.1s\n", a.x)` with
`mov rsi, r12` instead of the address of `a.x`. Durable ownership therefore
moved to idea 74's byval-param pointer-materialization family.

## Suggested Next

Start plan step `1` for idea 74. Reconfirm the first owned seam in `fa_s1` and
nearby `fa_sN` helpers using the existing focused proof, generated
`00204.c.s`, and prepared metadata for `%p.a` / `%t2`, then narrow the first
repair packet to the shared authoritative-home decision that should feed
pointer-argument materialization for address-exposed `byval_param` values.

## Watchouts

- Preserve the verified advancement from idea 73: generated `myprintf` still
  stores `%lv.s` at `[rsp]` and calls `match(&s, ...)` with `lea rdi, [rsp]`.
- Do not reopen idea 72 or idea 73 unless fresh proof moves the first bad fact
  back into aggregate transport or `local_slot` permanent-home ownership.
- Do not force the new seam into idea 71 unless a later packet proves genuine
  `va_list` / `llvm.va_start` ownership after the earlier `fa_s1`
  pointer-materialization corruption is repaired.
- Preserve the concrete current bad pattern:
  `bir.call i32 printf(ptr @.str0, ptr %t2)` in prepared `fa_s1`, with `%t2`
  homed to `r12` even though `%p.a` has fixed byval stack slot metadata at
  offset `0`; emitted asm then becomes `mov rsi, r12`.
- `fa_s2` through `fa_s17` show the same `mov rsi, r12` shape in emitted asm,
  so any repair must stay semantic and family-level rather than helper-name
  specific.

## Proof

Lifecycle close and switch used the focused proof scope:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  after staying beyond the old `match()` crash; the first bad observable fact
  is now the wrong `Arguments:` output from `fa_s1`, not a `myprintf`
  `local_slot` crash
- Proof log path: `test_after.log`
Close-time regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
Result:
- guard passed
- before reported `1` passed / `1` failed / `2` total
- after reported `1` passed / `1` failed / `2` total
- no new failing tests on the matched scope
