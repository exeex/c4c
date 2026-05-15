Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Specify TLS Materialization Facts

# Current Packet

## Just Finished

Step 7 specified and carried explicit TLS address-materialization facts through
prepared data and AArch64 selected records without enabling terminal TLS
assembly output.

- Prepared `TlsGlobal` address materializations now carry a structured
  local-exec thread-pointer-relative model, the AArch64 thread-pointer register
  fact `tpidr_el0`, and explicit high/low TLS relocation identities
  `tprel_hi12` and `tprel_lo12_nc`.
- Prepared dumps expose those TLS facts beside the existing symbol, policy,
  address-space, and TLS booleans.
- AArch64 `AddressMaterializationRecord` now preserves the TLS model,
  thread-pointer register, and relocation identities from prepared facts.
- AArch64 selection for `TlsRelative` fails closed when TLS address-space facts
  or thread-pointer-relative relocation facts are missing; it does not infer TLS
  behavior from symbol spelling or storage-class text.
- Terminal TLS printing remains deferred and continues to reject selected TLS
  records with an explicit diagnostic.

## Suggested Next

Execute Step 8 from `plan.md`: implement the printer-owned terminal address
materialization output for the selected record kinds whose relocation sequences
are now fully specified, while keeping unsupported cases fail-closed.

## Watchouts

- Terminal TLS printer output is still deferred; this packet only made TLS facts
  explicit and selected-record-visible.
- TLS facts currently model the local-exec thread-pointer-relative AArch64 form
  from the legacy surface: `tpidr_el0`, `tprel_hi12`, and `tprel_lo12_nc`.
- Do not revive the archived implicit `x0` scratch convention; result authority
  remains the selected record's prepared result register.
- Direct global, string constant, label, and GOT selected-record behavior should
  remain stable except where printer work consumes the structured fields.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed, 139/139 backend tests green. Proof log: `test_after.log`.
