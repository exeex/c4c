Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Direct-Global Authority Contract

# Current Packet

## Just Finished

Completed Step 2: defined the direct-global return/select-chain authority
contract for idea 440 and recorded supporting notes under
`build/agent_state/440_step2_direct_global_contract/`.

Accepted direct-global return facts:

- Return use is producer-owned: the BIR terminator returns a pointer value with
  direct semantic global identity; raw `bir.ret ptr @global` spelling is
  evidence only.
- The returned value has a valid semantic global `LinkNameId` and prepared
  value id/home/storage for that same identity.
- The home/storage is target-consumable for scalar pointer return, currently a
  GPR/register home for the RV64 packet.
- A `BeforeReturn` move bundle exists for the same block/instruction with
  `destination_kind=FunctionReturnAbi`, the same source value id, and a
  concrete return ABI register/bank.
- A new explicit direct-global return authority predicate/fact must accept the
  shape; `authority=none` return moves plus raw BIR are not enough.

Accepted direct-global select-chain facts:

- `PreparedDirectGlobalSelectChainDependency` is available and
  `contains_direct_global_load=true`.
- The consumer source value has a valid prepared value name/home/storage.
- The root producer chain reaches a semantic `load_global` of a valid global
  symbol; select shape or dump text alone is insufficient.
- `root_is_select` and `root_instruction_index` identify the prepared producer
  route, but consumers must use the prepared dependency rather than raw index
  matching.
- Call arguments may consume the dependency only through prepared call argument
  routing; generic terminator/select publication remains in idea 441.

Rejected adjacent shapes:

| shape | disposition |
| --- | --- |
| raw `bir.ret ptr @global` without explicit prepared return authority | rejected |
| computed pointer, `inttoptr`, null, local/frame-slot pointer, or opaque pointer return | rejected for direct-global return authority |
| missing/mismatched value home, storage, symbol identity, or ABI return move | rejected |
| `direct_global_select_chain=yes` without a valid dependency and consumer route | rejected |
| generic branch/return/select lowering | idea 441 unless narrowed to explicit direct-global authority |
| store-global source/layout publication | completed by ideas 439/446/447/448; out of scope |
| testcase/function/symbol-name matching | rejected as overfit |

## Suggested Next

Execute Step 3: add focused producer/prepared contract coverage for
direct-global return authority first. Suggested owned files/tests:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/value_locations.hpp` only if an existing move lookup
  helper needs a small reuse hook
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp` or
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp` only if a new prepared
  dump fact is introduced
- `todo.md`
- `test_after.log`
- `build/agent_state/440_step3_direct_global_return_authority/*`

The first Step 3 packet should create or confirm an explicit predicate/fact
that accepts the coherent direct-global return shape and rejects raw-only,
missing-home, missing-ABI-move, non-global pointer, computed pointer, and
mismatched-value-id shapes. It should not implement generic RV64 terminator
lowering.

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to direct-global return/select-chain authority.
- Do not fold general terminator/select publication into this plan; that
  belongs to `ideas/open/441_terminator_select_publication_authority.md`.
- Do not infer return authority or select-chain roots from raw
  `bir.ret ptr @global`, symbol spelling, select shape, testcase names, or one
  dump layout.
- Keep missing or incoherent direct-global authority fail-closed.
- Treat existing `direct_global_select_chain=yes` rows as candidate facts until
  Step 2 defines the accepted authority predicate and consumer boundary.
- Keep completed store/global publication facts from ideas 446/447 out of the
  direct-global packet.
- Do not treat `BeforeReturn` move bundles with `authority=none` as sufficient
  direct-global return authority without the new explicit predicate/fact.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log` or `test_after.log` during this contract-only
  packet.

## Proof

Step 2 contract-only validation:

```sh
git diff --check
```
