# Execution State

Status: Active
Source Idea Path: ideas/open/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Authoritative Byval-Home Pointer Materialization Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Plan step `2` narrowed the `%t2` seam to an exact blocker and removed the
uncommitted heuristic attempt from `src/backend/prealloc/regalloc.cpp` and
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`. Fresh prepared
inspection shows `fa_s1` still calls `printf(..., ptr %t2)` while `%t2` has no
defining pointer op in dumped BIR, no `PreparedMemoryAccess` entry, and only a
plain register home in prepared value locations; the handoff therefore lacks a
semantic artifact that names which authoritative byval home `%t2` came from.
With the speculative route removed, the helper-lane regression is gone, but the
focused `00204.c` proof remains blocked at the original runtime mismatch.

## Suggested Next

Keep plan step `2` active, but route the next packet to the upstream producer
that synthesizes call-site pointer names like `%t2`. That packet should expose
an explicit semantic link from `%t2` to its authoritative owner, such as a real
pointer-defining op or prepared artifact that records the base value and byte
delta, before regalloc or x86 rendering tries to consume it.

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
  offset `0`; emitted asm still becomes `mov rsi, r12`.
- `fa_s2` through `fa_s17` show the same unresolved `mov rsi, r12` family, so
  any repair must stay semantic and family-level rather than helper-name
  specific.
- Current blocker evidence is upstream of regalloc and x86 rendering:
  `--dump-bir` shows `bir.call i32 printf(ptr @.str0, ptr %t2)` in `fa_s1`
  without any visible defining pointer op for `%t2`.
- Matching prepared evidence stays incomplete: `--dump-prepared-bir` reports
  `home %p.a ... kind=stack_slot slot_id=0 offset=0`, `home %t2 ... kind=register reg=r12`,
  and a before-call move from value id `5` into ABI arg `1`, but
  `prepared-addressing` only records `%p.a`-based byte loads into
  `%lv.param.*` copies and no `%t2` access or `%t2` ownership record.
- Because the handoff never records `%t2`'s base owner or byte delta, step `2`
  cannot be completed semantically inside the owned files without inventing the
  missing provenance.
- The reverted heuristic path is still rejected for lifecycle purposes:
  do not infer `%t2` from the last seen byval pointer family, from a lone byval
  param, or from helper/function shape.

## Proof

Focused proof and seam confirmation used:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  at the same byval pointer-argument family
- Proof log path: `test_after.log`
Diagnostic inspection:
- `./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c`
- `./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c`
Result:
- dumped BIR still shows `%t2` only at the `printf` call site in `fa_s1`, with
  no visible defining pointer op that could be traced back to `%p.a`
- prepared dumping still shows `%t2` only as `kind=register reg=r12`, and
  prepared addressing still has no `%t2` access record, so the authoritative
  owner required by step `2` is not available in the owned seam
- the speculative regalloc/x86 carrier experiment was removed because it
  restored no semantic ownership and regressed the helper-lane proof
