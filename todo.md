Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh prepared/global authority inventory

# Current Packet

## Just Finished

- Completed Step 1 (`Refresh prepared/global authority inventory`) from
  `plan.md`.
- Wrote the selected prepared/global authority allowlist to
  `build/agent_state/608_step1_prepared_global_authority.allowlist` and the
  classification inventory to
  `build/agent_state/608_step1_prepared_global_authority_inventory.md`.
- Current producer-owned residuals are split into selected object-data
  authority (12 rows), prepared global memory facts (12 rows), and direct
  global-symbol base-plus-offset authority (11 rows). RV64 global symbol
  emission (17 rows), supported-width policy/consumer rows (13 rows), and
  idea `609` link/runtime/relocation residuals remain outside this producer
  packet.
- Selected the prepared global memory fact family as the first implementation
  family for Step 2, with `src/strlen-7.c`, `src/20000703-1.c`,
  `src/20041218-1.c`, and `src/pr58662.c` as the initial representatives.

## Suggested Next

Delegate Step 2 (`Publish supported prepared global memory facts`) to publish
supported prepared global memory access facts from proven producer inputs.
Suggested proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Keep this route in prepared/global authority. Do not fold RV64 global symbol
  emission, relocation-record emission, or access-width lowering into this
  plan.
- The selected object-data family is still visible in current logs, but Step 2
  should not repair object-data publication unless the supervisor deliberately
  changes the packet.
- Direct global-symbol base-plus-offset rows have a distinct missing-authority
  diagnostic and should stay reserved for Step 3.
- Do not change expectations, unsupported markers, allowlists, timeout or
  accounting files as progress.

## Proof

- Ran the delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Result: passed, 346 backend tests, 0 failures. Log path:
  `test_after.log`.
