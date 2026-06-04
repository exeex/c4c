Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 decomposed return/sret localization packet for the next
remaining AArch64 `00204.c` ABI family after direct `Arguments:` matched.

First wrong return/sret fact localized:

- The direct `Arguments:` section remains matched, and the first residual
  mismatch starts immediately after `Return values:`.
- A minimal focused route probe was attempted for a one-byte global aggregate
  return, but the only passing probe asserted the known stale-source behavior:
  `strb w13, [sp, #1]`, `ldrb w13, [sp, #1]`, `strb w13, [x8]`, with no
  direct global load before the stack store. That route target and fixture were
  removed so this checkpoint does not codify the bug as expected behavior.
- The same first wrong fact appears in `00204.c` at `fr_s1`: the BIR has
  `%t0.global.aggregate.load.0 = bir.load_local i8 ... addr s1`, but selected
  AArch64 still stores stale `w13` to the temporary frame byte before copying
  it to the hidden `x8` sret pointer.
- No broad c-testsuite expectations were changed and no `00204.c` special case
  was added.

## Suggested Next

Repair the localized SRET source publication fact: AArch64 lowering needs to
materialize same-block scalar `LoadLocal` producers whose address is a global
symbol before publishing aggregate return bytes to their temporary stack home
and hidden `x8` sret destination. After that, re-check whether the next
remaining return family is direct small-aggregate ABI classification or HFA
return lane lowering.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- No focused repair route target is committed for this checkpoint. The only
  focused probe available in this pass asserted stale SRET behavior, so it was
  removed rather than left as a passing expectation.
- Small non-HFA aggregate return classification may still be modeled as sret;
  do not paper over that with expectation rewrites.
- HFA return output is also still suspect, but it is downstream of the first
  stale byte publication fact.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. Expected for this localization-only checkpoint: selected
proof is back to the pre-localization test count, all selected backend route
tests pass, `backend_prepare_frame_stack_call_contract` passes, and guard cases
`00032.c`/`00182.c` pass. The only selected failure is
`c_testsuite_aarch64_backend_src_00204_c`; the first residual wrong fact remains
the stale SRET byte at the start of `Return values:`. No new route target
asserts the stale SRET behavior. Canonical executor proof log: `test_after.log`.
