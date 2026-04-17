Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Finished the last `plan.md` Step 2 semantic blocker on the selected
local-memory `gep` lane by teaching loaded local-array pointers to survive
both one-past-end and one-before-base scalar `gep` steps through the shared
`lir_to_bir` route. The narrowed proof for `backend_lir_to_bir_notes` plus
`c_testsuite_x86_backend_src_00032_c` now shows the same state already reached
by `00073` and `00130`: semantic `lir_to_bir` succeeds, and the remaining
failure is only the existing x86 prepared-handoff boundary outside owned
files. Step 2 is now semantically complete for this selected `gep` lane.

## Suggested Next

Move this lane to Step 3 x86-handoff work without reopening semantic lowering:
the selected `gep` cluster (`00032`, `00073`, `00130`) is now blocked only by
the prepared-module x86 emitter boundary, not by shared `lir_to_bir`.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- Keep the first slice in the local-memory family with already-supported
  control flow.
- Shared `lir_to_bir` capability growth should explain the result more than any
  x86-local patching.
- `00032`, `00073`, and `00130` are now all outside this packet's semantic
  scope; they stop at the x86 prepared-handoff boundary rather than inside
  `lir_to_bir`.
- The semantic repair that closed `00032` was shared, not testcase-shaped:
  loaded local-array pointers now survive `gep` results at `base_index == size`
  and `base_index < 0` through the existing local-memory provenance maps.
- Leave `00037`, `00042`, `00046`, `00143`, and `00207` out of this packet;
  this Step 2 lane is now semantically complete and the next blocker is in the
  x86 handoff stage.

## Proof

Ran the narrowed delegated proof exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -R '^backend_lir_to_bir_notes$' --output-on-failure >> test_after.log 2>&1 && ctest --test-dir build -R '^c_testsuite_x86_backend_src_00032_c$' --output-on-failure >> test_after.log 2>&1`.
`backend_lir_to_bir_notes` passed, and `00032` no longer fails in semantic
`lir_to_bir`; it now fails only at the x86 prepared-handoff boundary. Local
spot checks with `./build/c4cll --codegen asm --backend-bir-stage semantic`
confirmed that `00032`, `00073`, and `00130` all lower through semantic BIR on
the repaired shared `gep` lane. The packet should still stay uncommitted until
the Step 3 x86 handoff work is chosen.
Proof log:
`test_after.log`.
