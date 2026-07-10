Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Select The First Repair Boundary

# Current Packet

## Just Finished

Step 2: Select The First Repair Boundary completed a selection-only handoff
from the Step 1 evidence in `test_after.log`. Selected implementation
boundary: prepared contract production for the helper-built aggregate `va_arg`
AP pointer call-argument ABI destination publication.

Owned row for the first repair packet:

- `backend_prepare_liveness`

Positive evidence to preserve:

- `backend_prepare_liveness` currently fails first with
  `expected helper-built aggregate va_arg ap pointer to publish the concrete ABI argument destination`,
  which points at a missing prepared producer fact for the concrete
  call-argument ABI destination on the helper-built aggregate `va_arg` AP
  pointer move.
- The selected repair should publish that destination as a prepared contract
  fact, not as CLI wording, diagnostic relabeling, expectation churn, or a
  named-case shortcut.

Negative evidence to preserve:

- `backend_prepare_frame_stack_call_contract` reaches RV64 FPR preserved-value
  and fixed-frame saved-slot facts before the printer-facing failure
  `prepared dump hides FPR facts`; keep it out of the first implementation
  packet because Step 1 classifies it as printer formatting over existing
  prepared contract facts.
- `backend_prepared_printer` and `backend_prealloc_inline_asm` fail first on
  the f128 carrier text `value_id=0` while refreshed output contains
  `f128_carrier p.value value_id=1 ...`; keep both out because this is stale
  value-id exposure/expectation evidence, and the inline-asm row fails before
  any inline-asm-specific owner is proven.
- `backend_cli_dump_prepared_bir_exposes_contract_sections` exposes the
  requested `block_entry_publication successor=logic.end.4 ... to=%t8 ...
  reg=r11` row but with `to_value_id=10` rather than the expected
  `to_value_id=9`; keep it out as stale value-id/exposure evidence.
- `backend_cli_dump_prepared_bir_local_arg_call_contract` exposes the `%t1`
  callee-saved `rbx` storage row but with `value_id=4` rather than the expected
  `value_id=3`; keep it out as stale value-id/exposure evidence.

## Suggested Next

Delegate Step 3: Repair The Selected Contract Or CLI Rule for the
`backend_prepare_liveness` prepared producer boundary only. The focused proof
subset for the first repair packet should be:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_liveness$' > test_after.log 2>&1
```

The packet should repair the general prepared publication rule for
helper-built aggregate `va_arg` AP pointer call-argument ABI destination facts
and keep the stale value-id/exposure rows out unless new producer evidence
proves they share the same boundary.

## Watchouts

- Keep target-specific AArch64 dispatch, RISC-V object emission, RV64 runtime
  lowering, and LLVM torture discovery out of this route unless evidence
  requires supervisor lifecycle routing.
- Do not change expectations, unsupported markers, allowlists, runtime policy,
  timeout policy, baseline accounting, or test classification.
- Do not treat CLI text-only rewrites, helper renames, diagnostic relabeling,
  or classification-only edits as capability progress.
- For the first implementation packet, do not edit tests, expectations,
  unsupported markers, allowlists, runtime policy, baseline files, or the stale
  f128/CLI value-id exposure rows. Those rows need a separate review or
  producer-evidence packet, not this liveness repair.
- `backend_prealloc_inline_asm` currently does not prove an inline-asm
  publication owner because the shared executable fails earlier on f128 carrier
  value-id text.
- The local-arg CLI row exposes the requested sections and the same `%t1`
  storage placement, but current value numbering is shifted from expected
  `value_id=3` to actual `value_id=4`; treat that as stale exposure until a
  semantic producer gap is proven.
- The block-entry CLI row likewise exposes the requested
  `logic.end.4`/`%t8`/`r11` publication rows, but current value numbering is
  shifted from expected `to_value_id=9` to actual `to_value_id=10`; do not select
  it as a prepared contract production repair boundary without new evidence.

## Proof

No test rerun required for this selection-only Step 2 packet. Selection cites
the existing Step 1 evidence in `test_after.log`, produced by:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_inline_asm|backend_cli_dump_prepared_bir_exposes_contract_sections|backend_cli_dump_prepared_bir_local_arg_call_contract)$' > test_after.log 2>&1
```

Result recorded from Step 1: build completed with `ninja: no work to do`;
focused CTest failed 6/6 as expected for the evidence-refresh packet. Proof
log: `test_after.log`.
