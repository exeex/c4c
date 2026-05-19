# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Compare Closed Owner Boundaries

# Current Packet

## Just Finished

Step 3 compared the Step 2 classified backend-regex buckets from accepted
baseline `test_before.log` against closed owner artifacts for ideas 285
through 297. The current baseline still has 61 failures: 22 machine-printer, 8
`lir_to_bir` admission, 18 runtime nonzero, 12 runtime mismatch, and 1
timeout/hang.

Closed owners that remain valid and should not be reopened from this capture:

- 285 non-leaf LR preservation: still valid. Current failures are quick
  runtime outcomes, printer/admission failures, or the separate `00220`
  timeout; none show the old non-leaf `bl` plus bare `ret` timeout owner.
- 286 scalar call value semantics: still valid. `00116` and `00159` closure
  does not contradict this capture. `00159` is a runtime mismatch again, but
  Step 3 has no generated-code proof that scalar direct-call ABI values
  regressed.
- 287 string/global address external call lowering: still valid. Current
  string/stdio-looking residuals are runtime buckets without assembly evidence
  that string/global pointer arguments are again unmaterialized.
- 288 stack-frame SP alignment: still valid. The current bus-error/signal
  cases, including `00089`, are not evidence of the old unaligned frame owner
  without generated-frame proof; idea 288 already separated non-alignment
  function-pointer and runtime owners.
- 289 function-pointer indirect call values: still valid. `00089` is a
  runtime signal in the current capture, but closure proved function-pointer
  materialization for that representative; no fresh indirect-`blr` value-loss
  evidence contradicts the boundary.
- 290 scalar parameter ALU authority and 291 call-argument register authority:
  still valid. Current failures do not show the specific stale incoming
  parameter or prepared call-argument register contradiction from those
  closures.
- 292 scalar expression/control-value authority: still valid. Current runtime
  nonzero/mismatch cases may be adjacent scalar/control symptoms, but the
  baseline evidence is runner output only; reopening needs generated-code proof
  that consumers are again reading stale scalar fallback registers.
- 293 side-effect control-value publication authority: still valid. `00164`
  and `00169` now appear in the current mismatch bucket, but Step 3 has no
  generated-code contradiction of the focused closure. Treat them as residual
  runtime evidence requiring inspection, not automatic reopen.
- 294 pointer-derived address/lvalue lowering authority: still valid. Current
  pointer/address-looking runtime mismatches and nonzero cases are adjacent,
  but not enough to reopen the closed owner without generated address/lvalue
  proof. The current `lir_to_bir` GEP admissions are frontend semantic
  admission residuals, not the already-closed AArch64 runtime address/lvalue
  lowering route.
- 296 fused compare-branch operand forms: still valid. The current
  machine-printer bucket no longer contains the old fused compare-branch
  operand-form diagnostic. `00200` is a runtime nonzero/corrupted-output case,
  and `00207`, `00214`, and `00215` are scalar immediate printer residuals as
  recorded by the idea 296 closure.
- 297 local-memory `lir_to_bir` admission: still valid. `00046` is passing in
  the accepted baseline. `00140` and `00218` are printer residuals, and `00216`
  remains pointer-parameter/flexible-array aggregate projection rather than
  direct local-memory GEP admission. The remaining GEP admissions are the
  residual global/pointer/aggregate projection owners already excluded by
  idea 297.

Idea 295 is the active umbrella inventory, not a closed owner artifact, so it
has no closed boundary to compare here.

Adjacent-but-new residual owners visible from the Step 2 classification:

- Residual semantic `lir_to_bir` projection owner is the clearest Step 4 split:
  global scalar-array GEPs `00176` and `00181`, pointer-value/parameter GEPs
  `00182` and `00209`, global dynamic aggregate member GEPs `00195` and
  `00205`, and bootstrap/global aggregate semantics `00204`. Keep `00216` as a
  separate pointer-parameter/flexible-array aggregate projection boundary case
  inside or next to this family, not as local-memory admission.
- AArch64 machine-printer residuals are coherent but probably need a separate
  printer idea rather than reopening 296 or 297. Subfamilies are scalar
  opcodes not printable (`sub`, `div`, `mul`), integer cast nodes
  (`zero_extend`, `sign_extend`), non-encodable scalar immediates for
  `add`/`xor`/`and`, stack-slot store source scratch printing, and the
  `00140` call-boundary move printer residual.
- Runtime nonzero and runtime mismatch buckets remain observed outcomes only.
  They include prior representative names from closed owners, but Step 3 lacks
  generated assembly or proof evidence to assign them to a reopen or a focused
  semantic owner. Treat these as inspection candidates after the compile-stage
  residual owners are split.
- `00220` timeout/hang is a standalone timeout owner candidate if the
  supervisor wants hang quarantine before runtime inspection. Do not fold it
  into runtime mismatch/nonzero or closed LR timeout work without fresh
  evidence.

## Suggested Next

Proceed to Step 4 by splitting the residual semantic `lir_to_bir`
global/pointer/aggregate projection owner first. Suggested owner scope:
global scalar-array GEP, pointer-value/parameter GEP, global aggregate member
projection, bootstrap/global aggregate admission, and the `00216`
pointer-parameter/flexible-array aggregate projection boundary. Keep printer
residuals, runtime buckets, and `00220` timeout as separate follow-on owners
unless new evidence proves a shared semantic rule.

## Watchouts

- This is an umbrella inventory. Do not implement fixes before a focused owner
  is split and activated.
- Do not reopen ideas 285 through 297 from failing counts or recurring testcase
  names alone. Reopen needs generated-code, proof, or diagnostic evidence that
  contradicts a specific closure boundary.
- `00159`, `00164`, `00169`, and `00089` appear again in runtime buckets, but
  the current packet only inspected logs and closed artifacts. They are not
  reopen evidence yet.
- Treat residual global/pointer/aggregate GEP work, `00216`
  pointer-parameter/flexible-array aggregate projection, and machine-printer
  residuals as separate classification inputs, not automatic combined owners.
- The runtime nonzero and runtime mismatch groups need generated assembly or
  narrower probes before Step 4 should split them into semantic owners.
- `00220` timed out after `5.01 sec` in `test_before.log`; keep it quarantined
  from broad runtime proof until a timeout-specific owner exists or the
  supervisor delegates bounded hang inspection.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, implementation files, or
  canonical logs during lifecycle activation.
- `test_before.log` is the accepted focused baseline. `test_after.log` is
  absent at activation, and `test_baseline.log` is a pre-existing ignored root
  log.

## Proof

Inspection-only Step 3 packet. Sources inspected were accepted repo-root
`test_before.log`, the Step 2 classification already recorded in `todo.md`,
and closed idea artifacts `ideas/closed/285_*` through `ideas/closed/297_*`.
No tests were run and no canonical logs were modified.
