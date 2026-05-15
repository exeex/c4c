Status: Active
Source Idea Path: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Cache-Maintenance Carrier Facts

# Current Packet

## Just Finished

Plan Step 2 - Define Cache-Maintenance Carrier Facts completed one accepted
side-effecting, no-result AArch64 cache-maintenance representative:
`llvm.aarch64.dc.cvau`.

Implemented carrier authority:

- BIR semantic facts now include `CacheMaintenance`, `CacheDcCvau`, and
  `CacheAddress` fields for the DC CVAU representative.
- Lowering preserves a void-result pointer-address intrinsic with explicit
  `MemoryAddress` authority, default address space, side-effect-only cache
  behavior, no immediate operand, source callee spelling, and a prepared call
  boundary.
- Prepared intrinsic validation now accepts complete DC CVAU carriers only when
  the pointer address, void result, address memory facts, side effects, operand
  home, and call-plan shape are all present.
- Prepared printer diagnostics expose complete cache carrier fields, including
  memory address space, while malformed cache carriers remain missing.
- AArch64 dispatch and machine-printer tests prove the complete cache carrier
  remains non-selected and unprinted by the current selected-machine route.

## Suggested Next

Execute Plan Step 3 - Define Pause/Hint Carrier Facts for one accepted
side-effecting, no-result AArch64 pause/hint representative such as
`llvm.aarch64.hint`, with structured immediate authority and no selected-machine
support.

## Watchouts

- Keep this route limited to cache-maintenance, pause/hint, and
  builtin-address carrier authority.
- Do not add selected-machine records, dispatch support, or printer spelling in
  the carrier packets.
- DC CVAU support is carrier-only. Later machine-node routes must consume the
  structured cache carrier rather than rediscovering support from the intrinsic
  name or rendered text.
- Do not treat prepared address materialization or TLS metadata as
  builtin-address intrinsic authority. Existing direct/GOT/TLS global, string,
  and label address materializations already have their own carrier and
  selected-machine routes.
- Builtin-address should remain a recorded ownership boundary unless a later
  lifecycle decision names a concrete non-materialization representative.

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
