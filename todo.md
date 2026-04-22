# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Prepared/X86 Call-Lane Clobber Surface
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Plan step `1` confirmed the first idea-75 runtime break is consumer-side at
the prepared/x86 overlapping call lane, not a return to idea 74's owner
publication family. Focused semantic BIR still lowers the first bad calls as
`%t3 = bir.call i32 printf(ptr @.str16, ptr %p.a)` in `fa_s17` and
`%t1 = bir.call i32 printf(ptr @.str84, i64 %p.x)` in `pll`. Focused prepared
state keeps the incoming source values in `rdi` (`%p.a` in `fa_s17`,
`%p.x` in `pll`) while the string constants live in a different register
(`@.str16` in `rbx`, `@.str84` in `r11`), and the authoritative `BeforeCall`
bundles publish the same ABI destinations in both functions:
`abi_index=0 -> rdi`, `abi_index=1 -> rsi` (`fa_s17`: block `0`, instruction
`34`; `pll`: block `0`, instruction `0`). The first emitted clobber is still
`lea rdi, [rip + .L.str16]` followed by `mov rsi, rdi` in `fa_s17`, with the
same `lea rdi` then `mov rsi, rdi` order in `pll`, so arg0 rewrites the shared
`rdi` lane before arg1 reads the old source.

## Suggested Next

Repair the smallest shared consumer seam in
`render_prepared_block_direct_extern_call_inst_if_supported` so the generic
register-argument loop preserves overlapping `BeforeCall` sources before it
emits arg0 into `rdi` and arg1 from the old `rdi` home. Keep the next packet
at the shared prepared/x86 move-application layer, and add only the tightest
proof that protects this overlap contract if a bounded seam test falls out
naturally.

## Watchouts

- Keep idea 74 closed unless fresh proof moves the first bad fact back to
  byval-param owner publication.
- Do not route this back into idea 61 unless the case regresses to pre-assembly
  prepared call-bundle rejection.
- Do not force the new seam into idea 71 unless a later packet proves genuine
  `va_list` or `llvm.va_start` ownership after the overlapping call-lane
  clobber is repaired.
- `x86_codegen.hpp` is already publishing the authoritative ABI destinations;
  the break is in consumer-side move application/order, not in prepared
  `BeforeCall` metadata publication.
- `fa_s17` and `pll` both route through `local-slot-guard-chain`, and both
  reach the same shared consumer seam in
  `render_prepared_block_direct_extern_call_inst_if_supported`, so do not
  split the follow-up into pointer-only or `i64`-only special cases.
- Do not special-case `fa_s17`, `pll`, `printf`, or one `mov rsi, rdi`
  spelling; the repair must stay at the shared prepared/x86 call-lane layer.

## Proof

Fresh focused proof rerun for plan step `1`:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  after the prepared dumps and emitted asm reconfirmed the first bad fact at
  the shared overlapping call-lane clobber in `fa_s17` / `pll`
- Proof log path: `test_after.log`
