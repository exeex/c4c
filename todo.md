Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Reverted the incomplete owned-file code changes from the `plan.md` Step 3
`00210` packet after rerunning the delegated proof. The rerun proved that
`backend_lir_to_bir_notes` and `backend_x86_handoff_boundary` still pass, but
`c_testsuite_x86_backend_src_00210_c` still fails on `error: x86 backend
emitter only supports a single-function prepared module through the canonical
prepared-module handoff`. Local inspection of prepared BIR showed the sharper
route issue: the `printf` calls carry string-address provenance, but
`00210`'s `actual_function` calls still remain indirect in prepared BIR, so the
current Step 3 direct-call framing is suspect.

## Suggested Next

Implement the repaired `plan.md` Step 3 route with `00210` as the proving
anchor. The next executor packet should own the smallest prepared-BIR or x86
handoff change that preserves external string/global pointer-arg provenance and
admits the bounded same-module function-pointer indirect-call lane without
reopening broader `00189`-style indirect/global-function-pointer/variadic
plumbing. Keep `00189`, `00057`, and `00124` out of scope unless a new packet
explicitly reopens them.

## Watchouts

- Do not weaken `x86_backend` expectations to accept fallback LLVM IR.
- Do not add testcase-named shortcuts or rendered-text recognizers.
- The delegated proof now shows the external `printf` string-arg provenance was
  not the surviving gate for `00210`; the surviving gate is that the
  `actual_function` calls still arrive as indirect in prepared BIR.
- The active runbook has been repaired around that boundary: treat `00210` as
  a bounded same-module function-pointer indirect-call lane, not as a
  direct-call lane.
- Do not satisfy the next packet by merely deleting the
  `functions.size() != 1` rejection or by adding a testcase-shaped `00210`
  matcher. The route must match the real prepared-BIR family.
- Keep `00189`, `00057`, and `00124` out of this packet; they remain adjacent
  indirect-runtime, emitter/control-flow, and scalar-control-flow families.

## Proof

Delegated proof rerun:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00210_c)$' | tee test_after.log`

Result: `backend_lir_to_bir_notes` passed, `backend_x86_handoff_boundary`
passed, and `c_testsuite_x86_backend_src_00210_c` failed on the single-function
prepared-module gate. Proof log: `test_after.log`.
