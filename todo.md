Status: Active
Source Idea Path: ideas/open/309_aarch64_indirect_call_argument_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Indirect-Call Preservation Owner

# Current Packet

## Just Finished

Step 1 localized the `00189.c` failure to the AArch64 prepared call-plan /
call-boundary consumer surface, not to the closed extern-data PIC relocation
owner. The current assembly already materializes `stdout` through GOT, but
`main` still loses the outer indirect call setup around the nested `(*f)(24)`:
`fprintfptr` is loaded into `x20`, the nested call runs through `blr x13`, then
the final call reloads `stdout` into `x9`, moves `x20` into argument register
`x1`, loads `w2` from the saved-register area at `[sp, #8]`, and branches
through `x21`, which was only restored from the prologue save slot. The final
call is therefore missing the actual callee register, the `stdout` -> `x0`
argument move, the `.str1` -> `x1` materialization/move, and the nested result
`x0` -> `w2` handoff.

Read-only owner map:

- Producer authority: `src/backend/prealloc/call_plans.cpp` builds
  `PreparedCallPlan`, including `build_indirect_callee_plan`,
  `build_call_preserved_values`, per-argument source/destination facts, and
  before/after call bundles.
- AArch64 consumer authority: `src/backend/mir/aarch64/codegen/dispatch.cpp`
  lowers, orders, retargets, and records scalar call-argument producers,
  address materializations, before-call moves, the call itself, and after-call
  moves.
- Call lowering surface: `src/backend/mir/aarch64/codegen/calls.cpp` consumes
  the prepared plan through `lower_before_call_moves`,
  `lower_after_call_moves`, `make_indirect_callee_register`, and
  `lower_prepared_call_instruction`.

The likely repair is a value-liveness / call-sequence ordering fix, with
parallel-copy/temp preservation as the implementation mechanism if the prepared
before-call bundle contains overlapping sources and destinations. The key gap
is that dispatch retargets call-argument move sources from `scalar_state`, but
the indirect callee register selected by `lower_prepared_call_instruction` is
still taken directly from the prepared callee home, and live caller values that
must survive the nested call are not re-established into the final call's ABI
registers before `blr`.

## Suggested Next

Step 2 should repair the semantic AArch64 call-boundary path, not special-case
`00189.c`: make the final indirect call consume the current materialized callee
value and lower all outer arguments after nested-call results are available.
Concretely, inspect the prepared dump/records for `main` and add a focused
local backend test that models `outer_indirect(stdout_like, fmt_like,
inner_indirect(24))`, then repair either prepared liveness/call bundles in
`prealloc/call_plans.cpp` or the AArch64 dispatch retarget/order logic so the
final sequence is equivalent to preserving `fprintfptr`, reloading/moving
`stdout` to `x0`, materializing `.str1` to `x1`, moving the inner result to
`w2`, and branching through the preserved/current `fprintfptr` register.

## Watchouts

- Do not broaden this owner into direct multi-argument shuffle
  (`00181.c`/`00182.c`), direct vararg aliasing (`00200.c`), or
  address-of-local direct-call argument preparation (`00218.c`).
- Do not reopen idea 308 unless generated assembly again uses direct non-PIC
  relocation forms for externally binding data symbols.
- Do not change expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, CTest registration, proof logs, or test contracts.
- Reject filename-only, instruction-string-only, `stdout`-only, or
  `fprintfptr`-only fixes.
- Existing local coverage proves pieces, not this failure shape:
  `backend_prepare_frame_stack_call_contract_test.cpp` has indirect-call
  publication, cross-call preservation, stack preservation, and variadic
  nested dynamic-stack contracts; AArch64 MIR tests retain prepared call
  provenance and preserved-value effects. I did not find a local AArch64 test
  that combines an indirect callee, outer arguments live across an inner call,
  and final indirect variadic call ABI setup.
- Treat the `[sp, #8]` `w2` load as a red flag: that offset is also used for
  the prologue save/restore of `x21` in the current assembly, so Step 2 should
  verify whether the nested result's prepared home is missing, stale, or
  overlapping saved-register storage before choosing the final repair.

## Proof

No tests were rerun per packet instructions. Proof inputs were current
`test_before.log`, `tests/c/external/c-testsuite/src/00189.c`, generated
`build/c_testsuite_aarch64_backend/src/00189.c.s`, AST-backed symbol lookup via
`c4c-clang-tool-ccdb`, and read-only inspection of the relevant backend
lowering/tests. No `test_after.log` update was requested or made for this
localization-only packet.
