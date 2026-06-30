Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Direct-Global Packet

# Current Packet

## Just Finished

Completed Step 3: added focused prepared-side direct-global return authority
coverage and recorded supporting evidence under
`build/agent_state/440_step3_direct_global_return_authority/`.

Implementation summary:

- Added `PreparedDirectGlobalReturnAuthorityStatus`,
  `PreparedDirectGlobalReturnAuthorityInputs`,
  `PreparedDirectGlobalReturnAuthority`,
  `plan_prepared_direct_global_return_authority`, and
  `prepared_direct_global_return_authority_available` in
  `src/backend/prealloc/publication_plans.hpp/.cpp`.
- The predicate accepts only a pointer return value with semantic global
  `LinkNameId`, matching prepared value home/name/id, register-backed home, and
  a matching `BeforeReturn` `FunctionReturnAbi` GPR register move for the same
  block/instruction.
- Focused tests in `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
  cover the coherent direct-global return shape and fail closed for raw-only,
  missing-home, missing-ABI-move, non-pointer, unsupported-home,
  non-global/missing-identity, mismatched-value-id, and unsupported destination
  shapes.
- No RV64 generic terminator lowering or idea 441 select/terminator publication
  was implemented.

## Suggested Next

Execute Step 4: residual disposition and close-readiness review for idea 440.
Re-probe representative direct-global return/select-chain rows, verify the
new direct-global return authority predicate covers the intended prepared
facts, and classify any remaining object-route failure as direct-global,
select-chain, or out-of-scope idea 441 terminator/select publication.

## Watchouts

- Keep this plan limited to direct-global return/select-chain authority.
- Do not fold general terminator/select publication into this plan; that
  belongs to `ideas/open/441_terminator_select_publication_authority.md`.
- Do not infer return authority or select-chain roots from raw
  `bir.ret ptr @global`, symbol spelling, select shape, testcase names, or one
  dump layout.
- Keep missing or incoherent direct-global authority fail-closed.
- Treat existing `direct_global_select_chain=yes` rows as candidate facts until
  a later packet defines a consuming predicate or routes them to Step 4
  disposition.
- Keep completed store/global publication facts from ideas 446/447 out of the
  direct-global packet.
- Do not treat raw `BeforeReturn` move bundles as sufficient outside
  `plan_prepared_direct_global_return_authority`.
- Do not accept or modify `test_baseline.new.log`.
- Keep generic branch/return/select lowering out of this idea unless a
  follow-up is explicitly narrowed to direct-global authority consumption.

## Proof

Step 3 implementation validation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
