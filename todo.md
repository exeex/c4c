# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm The Prepared/X86 Call-Lane Clobber Surface
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Lifecycle switch only: idea 74 closed after the owned byval-param
pointer-owner publication seam was repaired and `00204.c` advanced into a
later prepared/x86 call-lane clobber blocker. No executor packet has started
on plan step `1` yet.

## Suggested Next

Inspect the first overlapping call-lane clobber in `fa_s17` and `pll`, confirm
the authoritative prepared argument homes and BeforeCall bundle for the
affected `printf` lanes, and narrow the next packet to one generic
source-preservation seam.

## Watchouts

- Keep idea 74 closed unless fresh proof moves the first bad fact back to
  byval-param owner publication.
- Do not route this back into idea 61 unless the case regresses to pre-assembly
  prepared call-bundle rejection.
- Do not force the new seam into idea 71 unless a later packet proves genuine
  `va_list` or `llvm.va_start` ownership after the overlapping call-lane
  clobber is repaired.
- Do not special-case `fa_s17`, `pll`, `printf`, or one `mov rsi, rdi`
  spelling; the repair must stay at the shared prepared/x86 call-lane layer.

## Proof

Lifecycle switch only. Reuse the latest focused proof until the next executor
packet reruns it:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with `[RUNTIME_MISMATCH]`
  after the repaired owner-publication seam; runtime now diverges at the later
  prepared/x86 call-lane clobber
- Proof log path: `test_after.log`
