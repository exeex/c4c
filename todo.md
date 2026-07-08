Status: Active
Source Idea Path: ideas/open/609_rv64_global_data_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RV64 global consumer stops

# Current Packet

## Just Finished

- Completed Step 1 (`Inventory RV64 global consumer stops`) by writing
  `build/agent_state/609_step1_global_consumer.allowlist` and
  `build/agent_state/609_step1_global_consumer_inventory.md`.
- Selected allowlist rows: `src/20010924-1.c`, `src/pr61517.c`,
  `src/pr57877.c`, `src/pr57860.c`, `src/20030224-2.c`,
  `src/20020118-1.c`, `src/20020213-1.c`, `src/20000703-1.c`, and
  `src/pr82387.c`.
- Classified representatives as relocation-only object data (`20010924-1.c`),
  mixed/selected object data (`pr61517.c`, `pr57877.c`, `pr57860.c`,
  `20030224-2.c`), symbol-emission (`20020118-1.c`), access-width
  (`20020213-1.c`), and producer/authority boundary controls
  (`20000703-1.c`, `pr82387.c`).
- First owned RV64 consumer family for implementation is prepared
  mixed/selected object-data symbol/object publication, because four of the
  current five compact backend failures stop at
  `prepared selected object-data contract status=unsupported_but_coherent`.

## Suggested Next

Start Step 2 (`Consume prepared object-data symbol publication`) with
`src/pr61517.c` as the first representative, then check breadth on
`src/pr57877.c`, `src/pr57860.c`, and `src/20030224-2.c`. Use
`src/20010924-1.c` as the relocation-only guard and leave relocation-record
emission for Step 3.

## Watchouts

- Do not produce missing prepared/global authority in this plan.
- Do not infer object bytes, relocation slots, target identity, or access
  widths inside RV64 when prepared facts are absent.
- Keep expectation, unsupported-marker, allowlist, timeout, runtime/link, and
  accounting changes out of the proof.
- Do not treat `src/20000703-1.c` or `src/pr82387.c` as Step 2 repair targets
  unless refreshed evidence shows they moved past the producer/authority gates.
- The exact proof command for Step 2 is:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

## Proof

- Delegated Step 1 proof passed:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Result: 346 backend tests passed, 0 failed.
- Proof log path: `test_after.log`.
