Status: Active
Source Idea Path: ideas/open/443_closed_world_formal_pointer_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Focused Producer Coverage

# Current Packet

## Just Finished

Completed Step 3: implemented the first durable function-level formal pointer
authority surface and focused coverage.

Implementation:

- Added `bir::FormalPointerAuthorityKind` with `Unknown`, `InternalOnly`, and
  `NoExternalCaller`.
- Added `bir::Function::formal_pointer_authority`, defaulting to `Unknown`.
- Populated the field in LIR-to-BIR lowering from `LirFunction::is_internal`
  only: static/internal definitions receive `InternalOnly`; external-linkage
  and other non-internal definitions remain `Unknown`.
- Refactored the same-module computed-address formal provenance collector to
  consume `bir::Function::formal_pointer_authority` instead of private
  internal-name/internal-link-id sets.
- Preserved idea 442 behavior for internal/static callees and did not publish
  external-linkage `930930-1::f` provenance.

Focused coverage:

- `tests/backend/bir/backend_prepare_stack_layout_test.cpp` now checks the
  internal same-module computed-address formal fixture publishes
  `FormalPointerAuthorityKind::InternalOnly` and still proves the pointer-value
  memory accesses.
- The external-linkage variant checks the callee remains
  `FormalPointerAuthorityKind::Unknown` and pointer-value memory remains
  fail-closed.

Boundaries:

- No RV64 target lowering, expectations, unsupported markers, allowlists,
  runtime comparison files, or baseline logs were changed.
- Pointer-delta propagation remains out of scope until base formal authority is
  proven.

Artifacts:

- `build/agent_state/443_step3_closed_world_authority_surface/summary.md`

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Suggested owned files:

- `todo.md`
- `test_after.log`
- `build/agent_state/443_step4_residual_disposition/*`

Recommended Step 4 packet: re-check the `930930-1` prepared residual and decide
whether this idea should close as a completed internal-only authority-carrier
surface, remain active for a real `NoExternalCaller` producer, or route back to
idea 442 with the explicit note that external-linkage `930930-1::f` still
lacks no-external-caller authority.

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not publish formal pointer provenance for external-linkage callees from
  observed same-module direct callsites alone.
- Do not treat hidden/protected visibility, same-module call wrappers,
  `LinkNameId`, or `can_elide_if_unreferenced` as no-external-caller authority
  without a separately defined and tested contract.
- `FormalPointerAuthorityKind::NoExternalCaller` is a reserved accepted
  consumer state, but no producer populates it yet.
- Do not mark `930930-1` done while `%lv.param.reg1` and `%mr_TR - 8` rows
  remain `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Keep pointer-delta propagation behind proven base formal pointer authority.
- Do not accept or commit the rejected `test_baseline.new.log` full-suite
  baseline refresh with failing `string_authority_guard`.

## Proof

Step 3 delegated backend proof is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
