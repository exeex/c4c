# Current Packet

Status: Active
Source Idea Path: ideas/open/771_lir_automatic_local_label_address_table_initializer_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Preserve structured direct-constant authority through emission

## Just Finished

- Step 2 preserved a pointer-represented `DirectConstant` in generic
  `StmtEmitter::emit_set_assign_value` before any `coerce(rhs.str(), ...)`
  call, so each automatic `void *table[] = { &&first, &&second };` indexed
  element store retains its produced identity and the producer's pointer,
  current-function-owner, and target-label authority. The focused probe now
  verifies both element stores and the existing direct-rvalue malformed cases
  remain the nearby same-feature authority coverage.

## Suggested Next

- Step 3 should review the completed bounded route, preserve 768 Step 5 as
  the sole return point, and decide lifecycle handoff; do not absorb the
  stacked 768 producer packet.

## Watchouts

- This remains initializer-element production only: exclude later table
  `DeclRef` decay, carrier/`IndirBrStmt`, verifier, backend, Raw-BIR/importer,
  767, and 769. No additional malformed mutation was added at this consumer:
  its legal input is the pre-existing function-owned `DirectConstant`, and
  testing malformed owner/target/type/value identity here would require
  producer or verifier mutation outside this packet. The stacked direct-rvalue
  probe already rejects raw text plus invalid/foreign owner or target,
  non-pointer type, and missing/invalid/foreign produced identities.

## Proof

- `cmake --build --preset default` passed; `ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_label_address_rvalue_probe$'` passed;
  and `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' |
  tee test_after.log` passed (7/7). Proof log: `test_after.log`.
