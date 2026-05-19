# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Compare Against Closed Owners

# Current Packet

Compare the captured backend-regex failure buckets against closed AArch64
owners 285 through 294. Do not reopen closed work from counts alone and do not
implement fixes in this umbrella plan.

## Just Finished

- Step 3: compared the Step 2 buckets against closed AArch64 owners
  `285` through `294` using `todo.md`, `/workspaces/c4c/test_after.log`, and
  closure notes under `ideas/closed/`, without rerunning CTest or changing
  code, expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.
- Closed owners that remain valid from their closure notes:
  - `285` non-leaf LR preservation: current failures do not by themselves
    contradict the closed non-leaf `x30` frame rule. Current `00121`, `00175`,
    and `00196` are non-timeout failures, and `00220` is a timeout case that
    later owners explicitly kept separate from LR preservation.
  - `286` scalar direct-call value semantics: `00159` appears in the current
    runtime mismatch bucket, but the closure note says the ordinary scalar
    call-value subset passed; later owners treat residual `00159` as overlap,
    not as proof that the direct-call value owner reopened.
  - `287` string/global address external-call lowering: no current count alone
    proves the old string/global pointer argument materialization owner is
    open. Its starter subset was closed separately from scalar/local-state and
    timeout-sensitive cases.
  - `288` stack-frame SP alignment: current `00089` is a bus error and
    `00220` is a timeout, but `288` explicitly split remaining bus-error
    function-pointer cases to `289` and excluded timeout-sensitive cases such
    as `00220`; no current frame-alignment owner is established by count.
  - `289` function-pointer indirect-call values: current `00089` overlaps an
    old focused proof case that closure says passed. Treat that as requiring
    generated-code inspection before any regression claim, not as an automatic
    reopening of function-pointer value lowering.
  - `290` scalar parameter ALU authority and `291` call-argument register
    authority: their focused cases (`00124`, `00210`) are not in the current
    failed set, so the current buckets do not point at either closed owner.
  - `292` scalar expression/control-value authority: its starter cases are not
    sufficient to explain the current compile-stage printer/admission buckets.
    Current runtime cases need generated-code evidence before being assigned
    to this closed scalar/control owner.
  - `293` side-effect/control-value publication authority: current `00164`
    and `00169` overlap the closed proof subset and current `00159`, `00168`,
    and `00193` overlap cases the closure explicitly separated. These are
    possible stale-log/regression evidence only after generated-code review;
    they are not Step 5 candidates by count alone.
  - `294` pointer-derived address/lvalue lowering authority: its starter cases
    remain separated from the current compile-stage buckets. Current `00137`
    and `00138` are specifically documented in the closure note as residual
    string-byte compare return/control publication work, not pointer-derived
    address/lvalue authority.
- Current failures that overlap closed work but are already separated by
  closure notes:
  - `00137`, `00138`: separated by `294` as return/control-value publication
    after string byte loads and compares, not pointer-derived address/lvalue
    repair.
  - `00159`, `00168`, `00193`, `00196`: called out by `293`/`294` as
    closed-owner overlap unless fresh generated-code evidence contradicts
    closures `285` through `293`.
  - `00164`, `00169`: overlap `293` starters and should be treated as
    possible regression/stale-log evidence requiring inspection, not as a new
    owner from counts alone.
  - `00089`: overlaps `289` and the former `288` bus-error split; current bus
    error requires generated-code inspection before reopening either owner.
  - `00173`, `00220`: explicitly timeout-sensitive or timeout/hang separated
    by earlier owners; keep them outside printer/admission/runtime semantic
    owners until a focused hang owner is created.
- New focused owner candidates ready for Step 5:
  - Highest-value candidate: AArch64 machine-printer/lowering operand-form
    authority for fused compare-branch operands, covering the 22 compile-stage
    failures `00030`, `00034`, `00037`, `00038`, `00041`, `00054`, `00055`,
    `00057`, `00059`, `00076`, `00077`, `00085`, `00092`, `00093`, `00101`,
    `00127`, `00200`, `00203`, `00207`, `00212`, `00214`, `00215`. This is
    outside closed runtime owners `285` through `294` and should focus on
    semantic operand publication/printing, not filename matching.
  - Secondary compile-stage candidate: AArch64 scalar machine-printer
    operand-shape coverage for scalar casts, immediate range forms,
    stack-slot store sources, mul/div/rem operands, and immediate-left
    subtraction, covering the remaining 16 machine-printer failures. This may
    need sub-splitting before implementation because it contains multiple
    instruction-form families.
  - Secondary admission candidate: semantic `lir_to_bir` local-memory
    lowering/admission for `gep`, `store`, and `load` local-memory values,
    covering `00046`, `00140`, `00143`, `00157`, `00176`, `00181`, `00182`,
    `00185`, `00195`, `00205`, `00209`, `00216`, and `00218`. Keep
    `00204` separate as globals/aggregate bootstrap admission.
  - Runtime candidate after inspection: string-byte compare return/control
    publication for `00137` and `00138`, using the generated-code boundary
    already recorded by `294` rather than reopening `292` or `294` from
    counts alone.

## Suggested Next

- Step 4 should write the durable inventory findings and closed-owner boundary
  decisions into `ideas/open/295_backend_regex_failure_family_inventory.md`
  under `## Deactivation Note`, then Step 5 can split the highest-value
  focused idea around the 22-case fused compare-branch machine-printer family.

## Watchouts

- Do not treat all `backend` regex failures as one owner.
- Keep local backend/unit regressions separate from AArch64 external
  c-testsuite runtime failures.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration to improve counts.
- Do not reopen closed AArch64 owners 285 through 294 from counts alone.
- Current runtime overlaps with closed proof cases (`00089`, `00164`,
  `00169`) could be stale-log or true-regression evidence, but this packet did
  not inspect generated assembly and should not claim those owners are open.
- `00204` is not the same owner as the 13 local-memory `lir_to_bir` failures;
  keep globals/aggregate bootstrap admission separate unless later evidence
  proves a shared admission primitive.
- The runtime buckets remain symptom-based except where closure notes already
  supply a boundary, such as `294` separating `00137` and `00138`.

## Proof

Read-only comparison inputs:

```bash
/workspaces/c4c/test_after.log
ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
ideas/closed/286_aarch64_scalar_call_value_semantics.md
ideas/closed/287_aarch64_string_global_address_external_call_lowering.md
ideas/closed/288_aarch64_stack_frame_sp_alignment.md
ideas/closed/289_aarch64_function_pointer_indirect_call_values.md
ideas/closed/290_aarch64_scalar_parameter_alu_authority.md
ideas/closed/291_aarch64_call_argument_register_authority.md
ideas/closed/292_aarch64_scalar_expression_control_value_authority.md
ideas/closed/293_aarch64_side_effect_control_value_publication_authority.md
ideas/closed/294_aarch64_pointer_derived_address_lvalue_lowering_authority.md
```

Result: compared the Step 2 buckets against closed owner notes and identified
focused Step 5 candidates without rerunning tests. Proof log remains
`/workspaces/c4c/test_after.log`; no CTest rerun was performed for this
comparison packet.
