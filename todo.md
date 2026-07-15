# Current Packet

Status: Active
Source Idea Path: ideas/open/802_lir_switch_selector_type_reference_verifier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair anonymous layout / structured-call compatibility
你該做code review了

## Just Finished

- 802 Step 1 selector type-reference packet: validate structured selector refs
  before comparing their integer widths to the selector-owned definition;
  nearby coverage now rejects missing, non-integer, stale-width, and same-width
  incoherent selector refs without making display text authoritative.

## Suggested Next

- Supervisor: review the completed 802 Step 1 selector verifier/test packet
  and decide its lifecycle/commit handoff.

## Watchouts

- Width is the selector-owned integer semantic fact; `require_type_ref` still
  rejects malformed structured mirrors before that semantic comparison. Do not
  modify, accept, or broaden the preserved 801 call/aggregate work.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`; proof log:
  `test_after.log`.
