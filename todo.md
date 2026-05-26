Status: Active
Source Idea Path: ideas/open/41_riscv_prepared_edge_publication_dynamic_stack_source_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement Or Preserve Fail-Closed Dynamic Form

# Current Packet

## Just Finished

Step 2 preserved and made explicit the fail-closed policy for RISC-V
dynamic-address `StackSlot -> Register` edge-publication sources that lack
prepared dynamic address/scratch authority.

Updated `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
only. No emitter or shared prepared-authority code changed.

The focused test now proves:
- a `StackSlot` source with size 8 but no concrete `offset_bytes` remains
  `UnsupportedSourceHome`
- that dynamic-shaped `StackSlot` source does not render any load and does not
  record a source stack offset
- adding pointer-base fields to a `StackSlot` source without a concrete offset
  does not reclassify it as `PointerBasePlusOffset` materialization
- `PointerBasePlusOffset` sources remain address-value materialization, not
  dynamic stack-source loads
- large pointer-base deltas do not reuse the concrete large-offset stack-source
  load sequence

Existing concrete 4-byte, 8-byte, and large-offset `StackSlot -> Register`
behavior remains covered by the same focused test binary.

## Suggested Next

Proceed to Step 4 validation unless the supervisor wants an additional Step 3
negative-hardening packet. The dynamic-address authority gap is documented and
the fail-closed policy is now explicit in focused tests.

## Watchouts

Do not infer dynamic stack-source addresses from block shape, predecessor or
successor scans, fixture labels, value ids, stack slot ids, offsets, or test
names. Preserve existing concrete 4-byte, 8-byte, and large-offset behavior.
Do not treat `PointerBasePlusOffset` as a dynamic `StackSlot` load; it is
currently an address-value materialization path.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log`

Result: passed, 2/2 tests. Proof log: `test_after.log`.

`git diff --check -- tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp todo.md`
passed.
