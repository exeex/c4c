# Execution State

Status: Active
Source Idea Path: ideas/open/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Byval-Param Pointer-Argument Crash Surface
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Plan step `1` reconfirmed the owned idea-74 seam with fresh proof plus prepared
inspection. `fa_s1` still stores byval param `%p.a` in fixed stack slot `0`,
but prepared value locations classify `%t2` as plain register home `r12` and
the before-call bundle forwards that register into `rsi`; generated
`00204.c.s` therefore still emits `mov rsi, r12`. A narrow x86-side
authoritative-home lookup experiment did not change the emitted asm and was
reverted, so the next owned repair is upstream in prepared pointer-home
derivation rather than in final x86 call-argument rendering.

## Suggested Next

Start plan step `2` on the prealloc/regalloc ownership path. Trace why
prepared value-home classification leaves `%t2` as `kind=register reg=r12`
instead of a carrier that preserves `%p.a`'s authoritative byval stack home,
then repair that shared pointer-home derivation so fixed-arity helper call
arguments materialize from the same byval home seen in prepared stack layout.

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
- Prepared evidence now narrows the semantic owner further than the original
  plan note: in `--dump-prepared-bir`, `prepared.func @fa_s1` reports
  `home %p.a ... kind=stack_slot slot_id=0 offset=0` but
  `home %t2 ... kind=register reg=r12`, and the before-call move bundle sends
  value id `5` to ABI arg `1` in `rsi`.
- `prepared-addressing` for `fa_s1` only records `%p.a`-based byte loads into
  `%lv.param.*` copies; it does not record a separate `%t2` access, so the
  next packet should inspect `src/backend/prealloc/regalloc.cpp` pointer
  carrier/value-home derivation instead of adding more x86-side lookup cases.

## Proof

Focused proof and seam confirmation used:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  with the same post-link `fa_s1` family mismatch; generated asm still shows
  `mov rsi, r12` in `fa_s1` through `fa_s17`
- Proof log path: `test_after.log`
Diagnostic inspection:
- `./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c > /tmp/00204.prepared.txt`
- `./build/c4cll --trace-mir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c`
Result:
- prepared value locations and move bundles confirm `%t2` is already classified
  as register home `r12` before x86 emission
