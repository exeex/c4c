# Execution State

Status: Active
Source Idea Path: ideas/open/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Aggregate-Call Crash Surface
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Lifecycle review refreshed the focused `00204.c` runtime proof, confirmed
`backend_x86_handoff_boundary` still passes, and rehomed the active route out
of idea 71 after `gdb` showed the first post-link crash in fixed-arity helper
`fa_s2` rather than in a variadic `myprintf` / `va_list` path. Idea 72 is now
active for the fixed-arity aggregate-call runtime seam.

## Suggested Next

Start plan step `1` for idea 72 by inspecting the generated `fa_s2` crash
surface, confirming whether the first owned seam is aggregate argument home
selection, fixed-arity call setup, or helper forwarding, and then narrowing
the next packet to one owned aggregate-call runtime repair.

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

## Proof

Focused runtime probe for route rehome:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_before.log`
Result: `backend_x86_handoff_boundary` PASS; `c_testsuite_x86_backend_src_00204_c`
fails with `[RUNTIME_NONZERO] ... exit=Segmentation fault`.

Crash confirmation:
`gdb -batch -ex run -ex bt --args build/c_testsuite_x86_backend/src/00204.c.bin`
Result: first crash at `fa_s2`, called from `arg`, before the later variadic
runtime path.
