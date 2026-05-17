Status: Active
Source Idea Path: ideas/open/272_prealloc_schema_header_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory current schema families and dependency edges

# Current Packet

## Just Finished

Step 1 inventory completed for `src/backend/prealloc/prealloc.hpp`.

Ownership map:

- `names.hpp`: prepared ID aliases, `PrepareOptions`, `PrepareNote`,
  `PreparedNameTables`, name spelling/intern/resolve helpers, and the
  low-level `PreparedBirModule` forward declaration. Depends on BIR name ID
  constants, `TextTable`, and standard string/optional utilities.
- `frame.hpp`: stack objects, frame slots, stack layout, register-bank/slot
  placement enums, saved-register placement, frame plan, dynamic-stack plan,
  stack-frame offset helpers, and `stack_layout` helper declarations. Depends
  on names, BIR types/functions, target register-bank vocabulary, and standard
  vectors/strings.
- `addressing.hpp`: prepared addresses, TLS/materialization enums,
  materialization records, memory accesses, addressing functions, and lookup
  helpers. Depends on names, frame slot IDs, `PreparedValueId`, forward access
  to `PreparedValueHomeKind`, and BIR address-space/policy types.
- `liveness.hpp`: value kinds, live intervals, liveness values/blocks/functions,
  and liveness aggregate. Depends on names, prepared object/value IDs, and BIR
  type kinds.
- `regalloc.hpp`: register classes/group overrides, target register identity,
  allocation status, physical/stack assignments, allocation constraints,
  interference, ABI/move/spill records, regalloc values/functions, and the
  regalloc aggregate. Depends on names, frame IDs, liveness intervals,
  register placement/bank vocabulary from frame, and BIR value payloads.
- `value_locations.hpp`: value-home kind, move phases, value homes,
  move bundles, out-of-SSA authority helpers, value-location functions, and
  lookup helpers. Depends on names, frame IDs, regalloc move/binding records,
  register identities/placements, and BIR value payloads.
- `calls.hpp`: storage encoding enum where currently shared by calls/storage,
  call argument/result plans, clobbers, preservation routes, indirect callee,
  memory return, call plan/function aggregates, and call lookup helpers.
  Depends on names, frame IDs, register/move/storage placement types, value
  homes for some downstream plans, and BIR values.
- `variadic.hpp`: variadic entry register-save/overflow/va_list layout,
  helper resources, scalar/aggregate va_arg access plans, helper operand homes,
  variadic function aggregate, and lookup helper. Depends on names, frame IDs,
  BIR type kinds, and `PreparedValueHome`.
- `runtime_helpers.hpp`: f128/i128 runtime helper families, helper kinds,
  ABI/result/marshal policy enums, nested helper binding structs, helper
  function aggregates, and helper lookup helpers. Depends on names, call
  preservation/clobber records, carrier kinds, move phases/op kinds, value-home
  kind, register placement/class/bank types, frame IDs, and BIR opcode/type
  enums.
- `special_carriers.hpp`: i128/f128 carriers, atomic operation carriers,
  intrinsic carriers, inline asm carriers, carrier lookup helpers, and
  associated kind/name helpers. Depends on names, frame IDs, regalloc register
  vocabulary, value homes for intrinsic/asm operands, and BIR atomic/intrinsic/
  inline-asm value types.
- `control_flow.hpp`: prepare route, join/branch/parallel-copy schema, computed
  value and materialized-compare helper types, control-flow aggregate, and the
  large inline lookup/classification/render-contract helper set. Depends on
  names, BIR module/function/block/value/global types, unordered maps/sets,
  and some value-location/move authority vocabulary.
- `module.hpp`: `PreparedBirModule`, module-level `find_prepared_*` helpers,
  `BirPreAlloc`, and prepare entry declarations. Depends on every focused
  family above plus BIR module and target profile.

First extraction family: `names.hpp`.

Dependency constraints for the first extraction:

- Move the ID aliases, options/notes, `PreparedNameTables`, and name helpers as
  an intact low-level block before extracting any schema family that references
  `PreparedNameTables` or prepared IDs.
- Keep `PreparedBirModule` as a forward declaration in the low-level names
  layer until `module.hpp` can include all member-family headers.
- Keep `infer_call_arg_abi`, `BirPreAlloc`, and prepare entry declarations
  reachable through the `prealloc.hpp` compatibility umbrella, but do not force
  them into `names.hpp`.
- `names.hpp` must include real dependencies (`bir.hpp`, `text_id_table.hpp`,
  and needed standard headers) and must not include the umbrella.
- Preserve namespace, public type names, inline helper spelling, and table
  reattachment semantics exactly.

## Suggested Next

Execute Step 2 from `plan.md`: create `src/backend/prealloc/names.hpp`, move the
low-level names/options/notes block there, and update `prealloc.hpp` to include
it as the compatibility umbrella.

## Watchouts

- Keep this plan behavior-preserving.
- Do not change prepared output, public type names, or test expectations.
- Keep `prealloc.hpp` as the compatibility umbrella.
- Do not split implementation bodies as part of the schema header step.
- Ambiguous boundary: `PreparedStorageEncodingKind` is introduced just before
  call plans but is reused by storage plans and indirect callee plans; extract
  with `calls.hpp` only if later includes stay acyclic, otherwise use the
  lowest shared focused header that avoids recreating a helper monolith.
- Ambiguous boundary: register bank/placement types currently live with frame
  and saved-register schema but are reused broadly by regalloc, value homes,
  calls, carriers, and runtime helpers; move them before dependents or isolate
  as part of the low-level frame/register-placement family.
- Ambiguous boundary: control-flow helpers include substantial inline analysis
  over BIR and prepared name tables; keep them together initially rather than
  distributing tiny helpers across unrelated family headers.

## Proof

Inventory-only scratchpad update. No build proof or tests were run; no code,
header, test, or build files were changed. `test_after.log` was not produced
because the delegated proof was "No build proof required."
