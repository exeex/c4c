Status: Active
Source Idea Path: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Remaining Carrier Gaps

# Current Packet

## Just Finished

Plan Step 1 - Inventory Remaining Carrier Gaps completed a read-only carrier
inventory for the remaining AArch64 cache-maintenance, pause/hint, and
builtin-address families.

Current carrier surface:

- BIR semantic intrinsic authority is limited to `ScalarFpUnary`, `Crc`,
  `VectorMemory`, `VectorOperation`, and `Barrier`, with operations `FAbs`,
  `Crc32W`, `VectorLoad`, `VectorAdd`, and `BarrierDmb`.
- Prepared intrinsic carriers mirror those fields and only validate those
  families. The shared carrier can already preserve operation, feature,
  operand/result types, operand roles, vector shape, signedness, memory
  operand/access, barrier domain, immediate value, side effects, source callee,
  operand/result names, operand homes, result home, and call-plan presence.
- AArch64 semantic lowering currently recognizes only
  `llvm.aarch64.crc32w`, `llvm.aarch64.neon.ld1.v16i8.p0i8`,
  `llvm.aarch64.neon.add.v16i8`, and `llvm.aarch64.dmb`; unknown AArch64
  intrinsics fall through instead of creating carrier facts.

Per-family readiness:

- Cache-maintenance: no `CacheMaintenance` family, cache operation enum, cache
  operand role, or validator exists yet. A first representative should define a
  side-effecting, no-result pointer-address cache intrinsic such as a `dc`/`ic`
  maintenance form with explicit address operand, address space, memory access
  or side-effect-only memory behavior, immediate/operation identity where the
  intrinsic spelling encodes the op, prepared call-plan presence, and pointer
  operand home. This is implementable in this route.
- Pause/hint: no `Hint`/`Pause` family, hint operation enum, or immediate-role
  validator exists yet. A representative such as `llvm.aarch64.hint` can be
  modeled as a side-effecting, void-result immediate carrier with no result
  home and no memory operand; the validator needs accepted hint immediates and
  fail-closed handling for malformed or unsupported values. This is
  implementable after cache-maintenance.
- Builtin-address: existing prepared address materialization already owns
  direct global, GOT global, TLS global, string constant, and label address
  facts, including TLS `tpidr_el0` local-exec metadata and relocations. Those
  must stay outside intrinsic-carrier authority. Only a builtin that is itself
  an intrinsic call boundary, not a materialized global/TLS/string/label
  address, should become a builtin-address carrier. Current code has no such
  accepted representative, no `BuiltinAddress` family, and no validator. Treat
  builtin-address as blocked until a specific non-materialization
  representative is chosen by the supervisor/plan-owner; do not reuse TLS or
  address-materialization records as intrinsic support.

## Suggested Next

Execute Plan Step 2 - Define Cache-Maintenance Carrier Facts for one
side-effecting, no-result AArch64 cache-maintenance representative. Add the
semantic enum/operation/role facts, prepared validation, BIR and prepared
printer tests, and keep AArch64 dispatch/printer non-selecting.

## Watchouts

- Keep this route limited to cache-maintenance, pause/hint, and
  builtin-address carrier authority.
- Do not add selected-machine records, dispatch support, or printer spelling in
  the carrier packets.
- Do not treat prepared address materialization or TLS metadata as
  builtin-address intrinsic authority. Existing direct/GOT/TLS global, string,
  and label address materializations already have their own carrier and
  selected-machine routes.
- Cache-maintenance is the first implementable packet. Builtin-address should
  remain a recorded ownership boundary unless a later lifecycle decision names
  a concrete non-materialization representative.

## Proof

Delegated proof:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|prepared_printer|aarch64_instruction_dispatch|aarch64_machine_printer)'; } 2>&1 | tee test_after.log
```

Result: passed. Build was up to date and all 4 delegated tests passed:
`backend_lir_to_bir_notes`, `backend_prepared_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_machine_printer`.

Log path: `test_after.log`.
