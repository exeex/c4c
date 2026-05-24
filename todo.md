Status: Active
Source Idea Path: ideas/open/prealloc-stack-layout-contract-boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Clarify Header Contract Ownership

# Current Packet

## Just Finished

Step 2 - Clarify Header Contract Ownership completed the narrow header move.

Moved the stack-layout phase hook declarations and `FunctionInlineAsmSummary`
out of `module.hpp` into `stack_layout/stack_layout.hpp`, where the real
stack-layout contract now names object collection, coalescing hints, inline
asm summarization, regalloc hinting, and slot assignment.

Changed files:
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `todo.md`

Include pressure:
- `stack_layout/stack_layout.hpp` now directly includes `../frame.hpp` for
  `PreparedStackObject`, `PreparedFrameSlot`, `PreparedNameTables`, and their
  ID types.
- The first backend build showed
  `tests/backend/bir/backend_prepare_stack_layout_test.cpp` still reaches
  stack-layout hooks through aggregate `module.hpp`; tests are outside this
  packet, so `module.hpp` now includes `stack_layout/stack_layout.hpp` as a
  compatibility aggregate include while no longer declaring the hook block
  itself.
- No stack-layout implementation file needed a new include.

Behavior notes:
- No implementation semantics changed.
- `PreparedBirModule`, `BirPreAlloc`, frame data types, and frame-plan
  contracts stayed unchanged.
- No public forwarding header or broad include churn was created.

## Suggested Next

Step 3 - Split Coordinator Object And Slot Phases: inspect
`stack_layout/coordinator.cpp` after the header boundary move and perform only
one small behavior-preserving extraction if object collection, slot
assignment orchestration, or address publication has a clear file-local
boundary. If the remaining coordinator state is still too coupled, record the
no-code decision instead.

## Watchouts

- `module.hpp` still aggregates `stack_layout/stack_layout.hpp` for existing
  transitive consumers; a future cleanup may move those consumers to the real
  stack-layout contract directly, but that would need ownership over tests or
  broader include sites.
- Keep frame data structs in `frame.hpp`; moving them was not justified by
  this packet.
- Do not change stack object authority, frame sizing, slot assignment order,
  alignment, inline-asm stack facts, dynamic address semantics, or prepared
  printer frame output while splitting coordinator logic.

## Proof

Ran:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Result: passed, `162/162` backend tests.

Ran `git diff --check`; passed.

Proof log path: `test_after.log`.
