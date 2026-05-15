Status: Active
Source Idea Path: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Pause/Hint Carrier Facts

# Current Packet

## Just Finished

Plan Step 3 - Define Pause/Hint Carrier Facts completed one accepted
side-effecting, no-result AArch64 pause/hint representative:
`llvm.aarch64.hint(i32 1)`.

Implemented carrier authority:

- BIR semantic facts now include `PauseHint`, `HintYield`, and
  `HintImmediate` fields for the accepted hint/yield representative.
- Lowering preserves a void-result, side-effecting intrinsic with explicit
  immediate authority for value `1`, source callee spelling, and a prepared
  call boundary.
- Prepared intrinsic validation now accepts complete hint-yield carriers only
  when the i32-to-void shape, immediate value, side-effect-only facts, and
  call-plan shape are all present.
- Prepared printer diagnostics expose complete pause/hint carrier fields while
  malformed hint carriers remain missing.
- AArch64 dispatch and machine-printer tests prove the complete pause/hint
  carrier remains non-selected and unprinted by the current selected-machine
  route.

## Suggested Next

Execute Plan Step 4 - Define Builtin-Address Ownership And Carriers. Keep the
first packet limited to deciding whether this route owns any concrete
non-materialization builtin-address representative; otherwise record the
address-materialization/TLS boundary for plan-owner review.

## Watchouts

- Keep this route limited to cache-maintenance, pause/hint, and
  builtin-address carrier authority.
- Do not add selected-machine records, dispatch support, or printer spelling in
  the carrier packets.
- DC CVAU and hint-yield support are carrier-only. Later machine-node routes
  must consume the structured carriers rather than rediscovering support from
  intrinsic names or rendered text.
- Do not treat prepared address materialization or TLS metadata as
  builtin-address intrinsic authority. Existing direct/GOT/TLS global, string,
  and label address materializations already have their own carrier and
  selected-machine routes.
- Builtin-address should remain a recorded ownership boundary unless a later
  lifecycle decision names a concrete non-materialization representative.
- `llvm.aarch64.hint` malformed LIR cases can lower without producing
  `HintYield` authority in this harness; the Step 3 BIR test explicitly guards
  against producing a semantic carrier for unsupported immediate/result cases.

## Proof

Delegated proof:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|prepared_printer|aarch64_instruction_dispatch|aarch64_machine_printer)'; } 2>&1 | tee test_after.log
```

Result: passed. Build succeeded and all 4 delegated tests passed:
`backend_lir_to_bir_notes`, `backend_prepared_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_machine_printer`.

Log path: `test_after.log`.
