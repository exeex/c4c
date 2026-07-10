Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select One Object Contract Owner

# Current Packet

## Just Finished

Step 2 of `plan.md` selected exactly one first owner from the Step 1 evidence:
object writer contract.

Contract selected: RV64 prepared object-module publication for a prepared
function containing a successful variadic helper lowering must continue
consuming the remaining prepared instruction stream before publishing the
function text, symbols, and object module.

Positive state: after successful `va_start` helper lowering, a following
ordinary prepared instruction such as the `LoadLocalInst` for `%ap.addr`, with
matching prepared value-location and addressing facts, must be emitted into the
same function text before object-module publication.

Fail-closed negative states: helper lowering may reject malformed or incomplete
helper facts with a diagnostic, and ordinary prepared instruction emission may
reject missing or unsupported prepared addressing facts. A successful helper
lowering must not terminate RV64 function text emission early when subsequent
prepared instructions and their facts are present.

## Suggested Next

Step 3 should implement the object writer contract in the RV64 prepared
instruction traversal/text emission path: make successful variadic helper
lowering resume traversal of subsequent prepared instructions before function
object publication, then prove with the same focused
`backend_riscv_object_emission` subset. If source inspection shows the helper
is not the traversal terminator, route back for lifecycle review with the
observed owner boundary before editing adjacent object-emission areas.

## Watchouts

- Step 1 evidence rules out relocation emission, section layout, symbol
  publication, and ELF serialization as the first boundary for the selected
  fixture because those structures are valid for the stale helper-only text.
- Do not implement this as testcase identity, expected-byte rewriting,
  unsupported-marker changes, allowlists, timeout/runtime policy, or baseline
  accounting.
- Do not absorb later focused failures for relocations, byval, incoming
  formals, local memory, pointer-value stores, sret, call-argument ordering, or
  diagnostic exactness into Step 3 until the post-helper traversal contract is
  repaired or disproven.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest failed with the existing
`backend_riscv_object_emission` failures. The failing focused subset is expected
for this selection-only Step 2 packet, and `test_after.log` is the canonical
proof log.
