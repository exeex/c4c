Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement or Prove the Narrow x86 Agreement Path

# Current Packet

## Just Finished

Step 3, Implement or Prove the Narrow x86 Agreement Path, is reset after
review. `review/reviewA.md` judged the uncommitted Step 3 implementation route
as drifting from the source idea, so executors must not continue to Step 4 from
the current implementation diff.

Reviewer blockers to repair before any proof packet:
- the agreement gate must compare Route 3 against
  `PreparedEdgePublication::source_memory_access`, not raw
  `PreparedMemoryAccess`;
- the agreement predicate must not cast `SlotNameId` directly to
  `PreparedFrameSlotId`, because those ids are different domains;
- legacy no-`MemoryAddress` `LoadLocal` compatibility may remain, but it must
  not return success from the Route 3 agreement predicate or be counted as
  Route 3 agreement evidence.

## Suggested Next

Redo Step 3. The supervisor should first make sure the current uncommitted
`src/backend/mir/x86/module/module.cpp` diff is not committed as progress.
Either discard that implementation diff before redelegating Step 3 or delegate
a replacement packet that rewrites it in place.

The replacement Step 3 packet must:
- use `PreparedEdgePublication::source_memory_access` and its source-memory
  status as the prepared-side authority row;
- keep raw `PreparedMemoryAccess` as compatibility data only after the
  publication source-memory row agrees with Route 3;
- map Route 3 local-slot identity through prepared stack object/frame-slot
  metadata without raw id-domain casts;
- separate legacy no-`MemoryAddress` fallback from positive Route 3 agreement;
- search for bypassing selected x86 `LoadLocal` memory/source consumers and
  record any out-of-scope paths.

## Watchouts

- Do not advance to Step 4 until the Step 3 implementation diff is replaced or
  removed and the new packet proves the selected x86 path checks Route 3 and
  prepared source-memory publication agreement.
- The source idea and `plan.md` still require the same bridge; this is a route
  reset, not a source-intent change.
- Preserving legacy no-`MemoryAddress` behavior is allowed only as a
  compatibility path distinct from agreement proof.
- Public prepared lookup/status surfaces, route-debug strings, fallback names,
  helper/oracle names, and x86 output formatting were not changed.

## Proof

Lifecycle repair validation:

```bash
git diff --check -- todo.md
```

Result: passed.
