# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Overlapping Call-Lane Source-Preservation Seam
Plan Review Counter: 3 / 5
# Current Packet

## Just Finished

Plan step `2` advanced through the aligned same-module HFA/byval caller-side
payload-base seam and the direct-extern variadic FP lane handoff in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`.
This packet kept the healthy `align=1` string/byval route on its previous
exact-home lookup, but taught the aligned byval payload path to consult the
published local payload base before falling back to the stale root-home
address. As a result, the rebuilt `arg()` asm now passes `fa_hfa11` through
`fa_hfa22` on the copied local payload slots (`[rsp + 136]`, `[rsp + 140]`,
`[rsp + 148]`, `[rsp + 160]`, `[rsp + 176]`, `[rsp + 184]`) instead of the old
stale root homes (`[rsp + 6096]` and friends), while the earlier string/byval
`Arguments:` lines remain correct through `fa_s17`.
The remaining first-lane HFA loss was then traced to direct-extern variadic FP
register numbering, where
the renderer was placing the first promoted float argument after the format
pointer into `xmm1` instead of `xmm0`. The direct-extern variadic float/double
path now numbers SSE argument registers by FP position (`xmm0`, `xmm1`, ...)
without widening the generic selector, so the rebuilt `arg()` binary now prints
correct HFA float/double argument groups through `fa_hfa14` and `fa_hfa24`
instead of the previous `0.0 ...` first-lane corruption.
`backend_x86_handoff_boundary` remains green. The focused proof is still red,
but the owned HFA caller seam has advanced beyond the previous first-lane loss;
the repaired `Arguments:` and `Return values:` sections now reach the `stdarg`
string/aggregate consumer before the next visible mismatch, and the later HFA
long-double plus mixed aggregate / return / stdarg fallout remains red
downstream.

## Suggested Next

Stay in plan step `2`, still inside the same renderer file, but shift the next
packet to the remaining direct-extern aggregate seam after the repaired
float/double HFA handoff. The focused binary now reaches correct `fa_hfa11`
through `fa_hfa24` prints and reaches the `stdarg` block before diverging, so
the next packet should start at that first red `stdarg` string/aggregate
consumption seam and then trace the later HFA long-double and downstream
aggregate/return lanes only after that boundary is understood.

## Watchouts

- The helper-prefix contract remains a guardrail and is still green:
  `backend_x86_handoff_boundary` still passes after the renderer change.
- Do not widen the published-payload preference into the generic pointer helper:
  that regressed the healthy direct-extern `printf` pointer lanes inside
  `fa_s1` through `fa_s16` before it was backed out.
- Do not generalize the new variadic FP register override outside
  `render_prepared_block_direct_extern_call_inst_if_supported`; the local fix
  is specifically compensating for variadic SSE argument numbering on this
  renderer path.
- The remaining red path is narrower but still inside owned code. `fa_hfa11`
  through `fa_hfa24` now print correctly, and the proof's first still-bad
  boundary after the repaired argument and return-value sections is the
  `stdarg` block, before the later HFA long-double and mixed aggregate /
  return sections.

## Proof

Fresh focused proof for this renderer follow-up packet:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with
  `[RUNTIME_MISMATCH]`; the earlier string/byval caller seam stays repaired,
  and the HFA float/double caller path now prints correctly through
  `fa_hfa24`, but the first visible downstream mismatch in `test_after.log`
  after the repaired argument and return-value sections is the `stdarg` block;
  later HFA long-double and mixed aggregate / return output remains corrupted
- Proof log path: `test_after.log`
