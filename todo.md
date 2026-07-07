Status: Active
Source Idea Path: ideas/open/580_rv64_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reproduce Compare Publication Owners

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by confirming current scalar compare publication
owner facts from existing `550` salvage evidence for one simple compare row and
one select-consuming compare row.

- Simple compare representative: `src/20080529-1.c`, function `test`,
  `%t0 = bir.ne float %p.c, 0x00000000`, prepared result home `%t0` in GPR
  `t0`; current route diagnostic remains `unsupported_scalar_compare_publication`.
- Select-consuming representative: `src/loop-8.c`, function `bar`,
  `%t6 = bir.ne double %t5, 0x3FF0000000000000`, prepared result home `%t6`
  in GPR `s1`; `%t6` feeds `%t10 = bir.select ...`, with prepared select-chain
  materialization recorded, while the first semantic owner remains compare
  result publication.
- Saved Step 1 artifacts:
  `build/agent_state/580_rv64_scalar_compare_publication/step1/compare-owner-facts.tsv`
  and
  `build/agent_state/580_rv64_scalar_compare_publication/step1/summary.md`.

## Suggested Next

Execute `plan.md` Step 2 by adding focused RV64 object-emission coverage for
ordinary F32/F64 compare result publication into a prepared GPR home, including
a direct published result and a select-consuming/materialization consumer if
practical.

## Watchouts

- Keep this lane limited to ordinary F32/F64 compare result publication into
  prepared GPR homes.
- Do not mix in floating casts, variadic helpers, F128, long-double, or helper
  implementation work.
- Do not claim progress through unsupported-marker changes, expectation
  rewrites, route allowlist edits, named-case checks, or fixed-register
  shortcuts.
- Step 1 evidence supports semantic fixture construction without rerunning the
  representative routes.

## Proof

Evidence-only packet. Read existing `550` owner classification, Step 2 prepared
dumps, and current per-case logs; no route rerun and no root `test_after.log`
were required.
