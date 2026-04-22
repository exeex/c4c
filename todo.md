# Execution State

Status: Active
Source Idea Path: ideas/open/75_post_link_prepared_call_lane_clobber_runtime_correctness_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Classify The Same-Module Aggregate String/Mixed-Aggregate Boundary
Plan Review Counter: 0 / 5
# Current Packet

## Just Finished

Plan step `2.2` is now rewritten because the blocked packet proved the old
"repair the shared prepared/x86 call-lane layer" wording was too broad. The
remaining first bad `00204.c` facts are still the same-module aggregate
string/mixed-aggregate lines (`sAJ AJT JT4 T4g 4gy gyz`,
`AJT JT4 T4g 4gt gtG tGH`, and the mixed aggregate numeric line), but the
executor confirmed the blocker is a route boundary: the bounded helper lane
cannot preserve the current canonical helper fragments and also repair
`fa1`/`fa2` once the published helper byval homes already overlap at 8-byte
spacing.

## Suggested Next

Take the next packet on rewritten plan step `2.2` as a classification packet,
not another renderer tweak. Decide which of these is true:
1. idea 75 still owns the seam because the bounded helper contract itself is
too strict, so the next executor should update
`backend_x86_handoff_boundary` and the helper-lane canonical fragment shape
before retrying `fa1`/`fa2`
2. the first bad fact is upstream prepared byval-home publication/layout, so
the next executable route must rehome out of this helper-renderer packet and
hand the case to the upstream publication/layout lane

## Watchouts

- Keep the new source-bias contract tied to the full reserved outgoing call
  stack, not just `stack_arg_bytes`; that is what fixed the HFA long-double
  seam.
- The first failing aggregate-string calls (`fa1`/`fa2`) are on the bounded
  same-module helper lane, not the broader same-module direct-call packet that
  also owns byval pointer handling.
- Do not schedule another generic helper-renderer edit unless the packet first
  proves that the helper contract, not the upstream byval-home publication,
  owns the bad fact.
- The reverted helper-lane bias experiment made `backend_x86_handoff_boundary`
  fail with `bounded multi-defined helper same-module local byval call route:
  helper-prefix renderer did not emit the canonical helper local-byval call
  fragments`.
- The reverted helper-only outgoing-payload reserve experiment made
  `backend_x86_handoff_boundary` fail with
  `bounded multi-defined-function helper same-module byval i8x2 call
  prepared-module route: x86 prepared-module consumer did not emit the
  expected helper-lane asm fragment`, while also corrupting `00204.c`
  much earlier in `Arguments:`.
- The reverted broader same-module per-call temp experiment pushed `00204.c`
  to `[RUNTIME_NONZERO]` with `exit=Segmentation fault`.
- The generated `00204.c.s` shows the unresolved concrete fact: the `fa1/fa2`
  payload copies are emitted into caller-frame homes at `rsp+6160`,
  `rsp+6168`, `rsp+6176`, `rsp+6184`, ... so aggregates wider than 8 bytes
  overlap before the helper call.
- Do not reopen idea 74's importer/publication seam unless the prepared
  `stdarg` handoff loses its preserved `ptr byval(size=..., align=...)`
  metadata again; the current upstream question is specifically byval-home
  owner/offset layout for the bounded helper lane.
- The remaining runtime frontier is later than the repaired HFA long-double
  `Arguments:` block and is still inside the owned x86 consumer path.
- Do not skip straight to `Return values:` or `stdarg:` debugging while the
  aggregate-string/mixed-aggregate lines are still the first bad visible facts.

## Proof

Fresh focused proof for this blocked step-2.2 x86 consumer packet:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
Current proof state:
- `backend_x86_handoff_boundary`: passed
- `c_testsuite_x86_backend_src_00204_c`: failed with
  `[RUNTIME_MISMATCH]`; the first visible mismatch is still the same-module
  aggregate string/mixed-aggregate seam (`sAJ AJT JT4 T4g 4gy gyz`,
  `AJT JT4 T4g 4gt gtG tGH`, and the mixed aggregate numeric line), followed by
  corrupted `Return values:` and `stdarg` output
- Proof log path: `test_after.log`
