Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Specify TLS Materialization Facts

# Current Packet

## Just Finished

Lifecycle close/switch completed: prerequisite idea 247 is closed after
explicit GOT policy reached target options, BIR symbol metadata, prepared
address-materialization facts, and selected AArch64 `GotPageLow12` records.
Idea 233 is active again with completed direct, label, and GOT-policy/selected
record progress preserved.

## Suggested Next

Execute Step 7 from `plan.md`: specify and carry explicit TLS materialization
facts so selected TLS records expose thread-pointer-relative policy and
relocation operands before any terminal TLS printer output is enabled.

## Watchouts

- Terminal GOT and label printing remain deferred to Step 8; do not format
  them from names.
- Do not infer TLS behavior from symbol spelling or storage-class text.
- Keep direct global, string constant, label, and GOT selected-record behavior
  stable except for shared carrier coherence needed by TLS or printer work.

## Proof

Close gate for prerequisite idea 247 used canonical backend logs:
`test_before.log` and `test_after.log` both report 139/139 backend tests
passing. `c4c-regression-guard` passed with non-decreasing pass-count mode and
no new failures.
