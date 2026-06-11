Status: Active
Source Idea Path: ideas/open/184_phase_e_route1_producer_constant_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove route/prepared equivalence

# Current Packet

## Just Finished

Step 4 tightened the selected
`value_publication_may_read_register_index(...)` consumer coverage in the
focused AArch64 dispatch fixture. The test now compares the Route 1 publication
producer view with the prepared same-block producer oracle for the same-block
nonconstant binary producer, checks no-producer/home fallback, missing producer
fail-closed behavior, and future-producer fallback, then clears prepared
source-producer indexes to prove the migrated route-first consumer still sees
both recursive operand register dependencies and rejects an unrelated register.

## Suggested Next

Proceed to Step 5 handoff and contraction guard for the selected consumer,
including the residual prepared fallback/oracle surfaces and the fact that no
broad prepared API contraction was claimed.

## Watchouts

The migrated consumer intentionally does not treat `NoProducer` as an
authoritative negative; any incomplete Route 1 view still falls through to the
prepared helper/home behavior. Target register, home, storage, move, and
machine-record policy remain in the existing AArch64/prepared paths. Step 4
used a future-producer case as the cross-block/out-of-scope fail-closed proof
for this same-block consumer rather than adding route-wide behavior claims.

## Proof

Step 3's accepted proof was rolled forward to `test_before.log`.

Step 4 ran and passed:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.
