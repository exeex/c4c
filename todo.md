Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate the import spine and CFG publication

# Current Packet

## Just Finished

- Step 5 first bounded packet added the public `lower_lir_to_raw_bir()` entry,
  structured import diagnostics, and a real verified `RawBir` path for void
  declarations plus empty-block definitions using `LirRet`, `LirBr`, and
  `LirUnreachable`.
- The importer prevalidates module/function scope, creates blocks in LIR order,
  keeps label-to-`BlockId` mapping adapter-local, sets terminators through
  `FunctionBuilder`, and publishes only through foundation verification.
- Accepted definitions now also require unique `LirBlockId` values and a
  resolvable `function.entry` equal to the first LIR block, matching the
  bootstrap BIR's order-defined entry without reordering or reinterpretation.
- Unmigrated `.cpp` files below `bir/lir_to_bir/` are explicitly quarantined
  from the active CMake graph while the new top-level importer remains active.

## Suggested Next

- Continue Step 5 at the consumer seam: replace or remove the stale
  `prealloc/prealloc.hpp` dependency in `backend.hpp` without restoring legacy
  prealloc, then route the backend through the new import result.

## Watchouts

- Foundation and the importer deliberately reject every ordinary instruction,
  parameter, non-void return, conditional/switch/indirect terminator, and
  module side table until its semantic family lands atomically.
- Bootstrap BIR has no explicit entry field; non-first LIR entry blocks remain
  an explicit unsupported import until the schema can carry that distinction.
- The default build now passes the new importer seam and next fails outside
  this packet because `src/backend/backend.hpp` includes missing
  `prealloc/prealloc.hpp`; do not route around it by restoring legacy sources.

## Proof

- Packet-local C++17 runtime proof passed void declaration, minimal void-return
  definition, two-block branch order/successors, unreachable, duplicate and
  missing labels, and explicit rejection of non-void, parameters, variadic,
  globals, ordinary instructions, and unsupported terminators.
- Entry/identity repair proof additionally passed an explicit positive entry
  case and rejected duplicate `LirBlockId`, missing entry, and non-first entry.
- The CMake object target for `bir/lir_to_bir.cpp` compiled successfully;
  `git diff --check` and compile-database searches for quarantined importer and
  legacy sources passed.
- `cmake --build --preset default -j 2` reached the next external seam and
  failed because `src/backend/backend.hpp` includes missing
  `prealloc/prealloc.hpp`.
- The delegated owned/do-not-touch set excluded logs, so no `test_after.log`
  was written; temporary proof sources, outputs, and executable were removed.
