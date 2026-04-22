# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Prepared/X86 Call-Lane Clobber Surface
Plan Review Counter: 2 / 4
# Current Packet

## Just Finished

Plan steps `2` and `3` shrank the owned idea-75 family at the shared
direct-extern call-lane consumer. The x86 prepared renderer now preserves
overlapping GP argument sources before rewriting ABI destinations, so the
generated `fa_s17` and `pll` asm no longer emits the old clobbering
`lea rdi, ...` then `mov rsi, rdi` sequence; both sites now preserve the old
source in `r8` and materialize `mov rsi, r8` before `call printf`. Fresh
focused proof still fails `00204.c`, but the earliest remaining bad fact moved
downstream to the start of `arg()`: the first failing site is now the caller's
`fa_s1(s1)` handoff, where `build/c_testsuite_x86_backend/src/00204.c.s`
stores the small struct into a stack scratch byte and then emits
`lea rdi, [rsp]` before `call fa_s1`. That feeds an address-shaped temporary
instead of the by-value payload, so the case has graduated past the old
direct-extern overlap seam and into the next caller-side aggregate handoff
leaf.

## Suggested Next

Take the next packet on the caller-side aggregate-by-value lowering that feeds
fixed-arity internal calls in `arg()`, starting with the first `fa_s1(s1)`
site and the repeated `lea rdi, [rsp + ...]` pattern for the small-struct
family through the early `fa_s2`..`fa_s8` calls. The repair should preserve
the now-cleared direct-extern overlap and focus on getting the caller to
publish the by-value aggregate payload instead of an address-shaped temporary.

## Watchouts

- Treat the old `fa_s17` / `pll` `rdi -> rsi` overlap as repaired unless a
  fresh proof reintroduces it; the shared direct-extern renderer now preserves
  those overlapping sources explicitly.
- The new earliest failure is caller-side, not callee-side: `fa_s1` itself
  reads the register it was given, but `arg()` currently feeds it an address
  temporary rather than the aggregate value.
- The repeated `lea rdi, [rsp + ...]` pattern across the small-struct fixed
  calls suggests a shared lowering rule for internal by-value aggregate
  arguments, so avoid testcase-shaped fixes that only special-case one helper.
- Keep the next packet on this earliest aggregate handoff seam before revisiting
  later `Return values:`, `stdarg:`, or `MOVI:` corruption.

## Proof

Fresh focused proof for the accepted direct-extern overlap repair:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: still failed with
  `[RUNTIME_MISMATCH]`, but the old direct-extern `fa_s17` / `pll` overlap is
  cleared in the generated asm and the first remaining wrong line now comes
  from the caller-side `fa_s1(s1)` handoff at the start of `Arguments:`
- Proof log path: `test_after.log`
