# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Overlapping Call-Lane Source-Preservation Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Plan step `2` advanced through the later same-module larger-byval caller-side
payload-carrier seam in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`.
This packet widened the dedicated byval payload-address handoff helper to all
same-module byval-copy pointer args, while keeping symbol-backed byval carriers
on their direct symbol addresses. As a result, the rebuilt `arg()` asm now
passes the later string aggregates through their published payload bases
instead of stale root homes, and the focused `00204.c` `Arguments:` section is
correct through `fa_s17`: `0`, `12`, `345`, `6789`, `abcde`, `fghijk`,
`lmnopqr`, `stuvwxyz`, `ABCDEFGHI`, `JKLMNOPQRS`, `TUVWXYZ0123`,
`456789abcdef`, `ghijklmnopqrs`, `tuvwxyzABCDEFG`, `HIJKLMNOPQRSTUV`,
`WXYZ0123456789ab`, `cdefghijklmnopqrs`.
`backend_x86_handoff_boundary` remains green. The focused proof is still red,
but the owned later string/byval caller corruption is cleared and the
remaining mismatch now starts at the HFA families and later downstream uses.

## Suggested Next

Stay in plan step `2`, but move to the remaining HFA aggregate seam in the same
renderer file. The later string/byval callers are repaired now, but the
focused proof still diverges beginning at `fa_hfa11+`, where the HFA
float/double/long-double families still materialize zeros or NaNs and later
poison the mixed aggregate, return-value, and stdarg sections.

## Watchouts

- The helper-prefix contract remains a guardrail and is still green:
  `backend_x86_handoff_boundary` still passes after the renderer change.
- The stable same-module byval fix now covers the later string family too: the
  rebuilt `arg()` calls to `fa_s9` through `fa_s16` use the published stack
  payload bases, and `fa_s17` no longer hands `rdi` an uninitialized stack
  placeholder.
- Do not reuse the rejected value-home-prefix scan route from this packet: it
  broke `backend_x86_handoff_boundary` and was reverted before the final proof.
- The remaining red path is narrower but still inside owned code. The HFA
  caller/callee families still disagree on which homes are authoritative, so
  the next packet should start from `fa_hfa11` through `fa_hfa34` rather than
  revisiting the later string/byval path again.

## Proof

Fresh focused proof for this renderer follow-up packet:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with
  `[RUNTIME_MISMATCH]`; the owned later string/byval caller seam is repaired,
  but the focused binary still prints corrupted HFA families and downstream
  mixed aggregate / return / stdarg output
- Proof log path: `test_after.log`
