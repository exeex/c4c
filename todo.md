# Execution State

Status: Active
Source Idea Path: ideas/open/74_post_link_byval_param_pointer_argument_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Authoritative Byval-Home Pointer Materialization Seam
Plan Review Counter: 2 / 4
# Current Packet

## Just Finished

Plan step `2` now publishes truthful lowering-side owners for the full
`fa_s1`..`fa_s17` pointer-call family inside the owned lowering seam. Fresh BIR
inspection shows the repaired public handoff:
- `fa_s1` through `fa_s16` no longer call `printf(..., ptr %t2)`; they now call
  `printf(..., ptr %lv.param.a.0)` from the authoritative local byval-home leaf
- `fa_s17` no longer calls `printf(..., ptr %t2)`; it now calls
  `printf(..., ptr %p.a)` from the incoming aggregate-param home
- the focused handoff regression continues to pass, including new bounded
  checks for both the local byval-home and byval-param zero-offset pointer-call
  routes

The delegated proof no longer fails at the idea-74 lowering seam. It now lands
on a later runtime blocker outside the owned lowering files: generated asm for
the repaired `fa_s17` call lane still emits `mov rsi, rdi` after loading the
format string into `rdi`, so the arg1 pointer is copied from the clobbered
format register instead of the authoritative `%p.a` owner. The same prepared
call-lane clobber also appears in unrelated functions such as `pll`, which is
why the full `00204.c` runtime is still broadly wrong even though the byval
pointer-argument owner publication is repaired.

## Suggested Next

Treat plan step `2` as complete enough to hand off. The next packet should move
to the prepared/x86 call-lane consumer that resolves overlapping call-argument
homes when arg0 already rewrites `rdi` but arg1 still sources from the original
`rdi` home. That packet should keep the repaired semantic BIR owners intact and
fix the later move-bundle or renderer route so `fa_s17` and `pll` no longer
emit `mov rsi, rdi` from a clobbered source.

## Watchouts

- Preserve the verified advancement from idea 73: generated `myprintf` still
  stores `%lv.s` at `[rsp]` and calls `match(&s, ...)` with `lea rdi, [rsp]`.
- Do not reopen idea 72 or idea 73 unless fresh proof moves the first bad fact
  back into aggregate transport or `local_slot` permanent-home ownership.
- Do not force the new seam into idea 71 unless a later packet proves genuine
  `va_list` / `llvm.va_start` ownership after the earlier `fa_s1`
  pointer-materialization corruption is repaired.
- Do not revert the new semantic owner publication in
  `src/backend/bir/lir_to_bir_memory.cpp`: the repaired handoff is now visible
  in dumped BIR and prepared BIR as `%lv.param.a.0` for `fa_s1`..`fa_s16` and
  `%p.a` for `fa_s17`.
- The new bounded regression in
  `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
  covers three distinct seams now:
  local byval-home owner publication, byval-param owner publication, and
  indirect aggregate-param owner publication.
- The live blocker has graduated downstream of idea 74. Current asm evidence:
  `fa_s17` lowers to `printf(ptr @.str16, ptr %p.a)` semantically, but the x86
  renderer still emits `mov rsi, rdi` after `lea rdi, [rip + .L.str16]`, which
  copies the format-string register into arg1 instead of preserving `%p.a`.
- The same later call-lane clobber appears outside the repaired family:
  `pll` also emits `mov rsi, rdi` after loading its format string, which
  explains the repeated wrong MOVI outputs in the full runtime proof.
- Keep the route semantic. Do not replace the later move-bundle problem with
  new lowering heuristics, helper-shape matching, or lone-byval inference.

## Proof

Focused proof and seam confirmation used:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Result:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  after the repaired owner-publication seam; runtime now diverges at the later
  prepared/x86 call-lane clobber
- Proof log path: `test_after.log`
Diagnostic inspection:
- `./build/c4cll --dump-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c`
- `./build/c4cll --dump-prepared-bir --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c`
- `./build/c4cll --codegen asm --target x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c`
Result:
- dumped BIR now shows the repaired semantic owners:
  `printf(ptr @.str0, ptr %lv.param.a.0)` in `fa_s1`,
  `printf(ptr @.str15, ptr %lv.param.a.0)` in `fa_s16`, and
  `printf(ptr @.str16, ptr %p.a)` in `fa_s17`
- prepared dumping for `fa_s1` now carries `%lv.param.a.0` as the call arg
  owner and marks that leaf as `address_exposed=yes` with a permanent home slot
- generated asm for `fa_s1` now uses `lea rsi, [rsp + 1]`, confirming the
  small byval-home family no longer routes through synthetic `%t2`
- generated asm for `fa_s17` still emits `mov rsi, rdi` after loading the
  format string, so the next blocker is the later prepared/x86 call-lane route,
  not the repaired idea-74 lowering seam
