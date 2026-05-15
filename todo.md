Status: Active
Source Idea Path: ideas/open/250_i128_deferred_helper_family_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Lane, Result, And Memory-Return Ownership

# Current Packet

## Just Finished

Executed Step 3 by adding prepared/shared lane, scalar result, and
memory-return ownership facts for supported i128 conversion helper mappings.

Conversion helper records now distinguish the i128 carrier side from the scalar
FP side. FP-to-I128 helpers keep direct low/high i128 result ownership and
carry scalar FP source ownership from prepared value-home/register-bank facts.
I128-to-FP helpers carry source i128 low/high lane ownership and scalar FP
result ownership from prepared value-home/register-bank facts. Memory-return
ownership remains explicit and fail-closed for unsupported shapes; this packet
does not invent destination slot/offset ownership.

The prepared printer now dumps scalar conversion ownership beside existing
lane ownership. Focused tests prove complete FP scalar source/result ownership
and incomplete/deferred states without touching AArch64 selected-record or
terminal printer paths.

## Suggested Next

Execute Step 4 as the prepared/shared ABI/register-bank transition and
marshaling packet for the conversion helper family. Use the Step 3 ownership
facts to define helper argument/result ABI bindings and source/result
marshal/unmarshal moves for scalar FP <-> i128 carrier transitions. Keep
memory-return-required and F128/binary128 families fail-closed unless explicit
producer authority is added, and do not add AArch64 selected nodes or terminal
printer output unless delegated.

Suggested focused proof for Step 4:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Do not reopen direct-result div/rem helper-call printing unless a concrete
  regression is found.
- Conversion helpers now have source-operation/helper mapping records plus
  lane/scalar ownership facts. Boundary policy, ABI register-bank transitions,
  marshaling, clobber/resource policy, live-preservation, and selected-call
  ownership remain incomplete by design.
- Do not treat the existing memory-return vocabulary as ownership. I128 helper
  memory-return support still lacks destination/slot/offset producer authority
  and helper ABI policy tying an sret/memory destination to the helper record.
- Do not hard-code helper ABI registers, fixed low/high lane pairs, register
  adjacency, rendered names, or helper callee strings in AArch64 target
  lowering.
- Do not lower helper-required operations as scalar i64 shortcuts.
- Preserve existing supported div/rem helper-call behavior while adding
  deferred helper authority.

## Proof

Focused proof passed:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

Result: 3/3 tests passed. `git diff --check` also passed.

Supervisor full-suite acceptance also passed: supervisor ran
`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`
and the regression guard against `test_before.log` copied from
`test_baseline.log` passed with before 3167/3167 and after 3167/3167.
