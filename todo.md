Status: Active
Source Idea Path: ideas/open/186_bir_direct_symbol_identity_validation_closure.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory direct-symbol validation surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for idea 186. Direct-symbol surfaces and current
validation behavior:

- `bir::CallInst`: carries `callee` plus `callee_link_name_id`. Validation
  accepts invalid ids for indirect calls and runtime/intrinsic placeholders,
  validates known ids through `module.names.link_names`, and rejects direct
  calls whose present id points at a different declared function than the
  visible callee name.
- `bir::LoadGlobalInst` and `bir::StoreGlobalInst`: carry `global_name` plus
  `global_name_id`. Validation checks known ids, resolves declarations by id
  when present, and rejects id/name pairings that name a different declared
  global. Invalid ids remain accepted only through declared-name lookup.
- `bir::Global` declarations: carry `name` plus `link_name_id`; validation
  checks known ids and rejects duplicate non-invalid global ids. Defined
  globals still require an initializer path.
- `bir::Function` declarations/definitions: carry `name` plus `link_name_id`;
  validation checks known ids and rejects duplicate non-invalid function ids.
- Pointer symbol values and pointer initializer symbol handling:
  `Value::pointer_symbol_link_name_id` and
  `Global::initializer_symbol_name_id` are validated against declared globals
  or functions and reject stale id/name pairings. LIR-to-BIR resolves
  `LirGlobal::initializer_function_link_name_ids` through
  `resolve_initializer_symbol_link_name_id`; a present-but-unknown function id
  fails closed instead of falling back to raw spelling.
- LIR-to-BIR compatibility indices remain raw-name keyed at the boundary:
  `GlobalTypes`, `FunctionSymbolSet::raw_symbol_link_name_ids`, and memory
  provenance/local-slot helpers bridge legacy LIR spellings to emitted BIR ids.

Selected Step 2 hardening target: generated metadata-rich direct calls plus
the paired pointer-initializer function-symbol path. For direct calls, a
direct global `LirCallOp` with structured `callee_signature` but missing
`direct_callee_link_name_id` can still lower to a direct `bir::CallInst` with
only a raw callee spelling; Step 2 should make generated metadata-rich
user/extern direct calls require a valid callee `LinkNameId` while preserving
explicit placeholder and raw/no-id compatibility. Pointer initializer handling
already fails closed for present `initializer_function_link_name_ids`; Step 2
should preserve that behavior and use it as the global-family proof target.

## Suggested Next

Run Step 2 on the selected direct-call boundary: distinguish generated
metadata-rich direct calls from raw/no-id direct-call compatibility and require
a valid `direct_callee_link_name_id` for the generated user/extern path. Keep a
focused pointer-initializer assertion around present
`initializer_function_link_name_ids` failing closed.

## Watchouts

- Runtime/intrinsic placeholder calls are intentionally invalid-id `CallInst`
  paths: variadic runtime helpers, inline asm placeholders, stack-state
  helpers, absolute-value intrinsics, and direct memory intrinsic lowering
  should not be reclassified as user/extern symbol identity.
- Raw/no-id compatibility routes to preserve: direct calls without structured
  callee metadata, declared-name lookup for BIR global loads/stores with
  invalid `global_name_id`, raw function/global lookup for legacy pointer
  initializers, string-pool globals, and route-local SSA/slot/block labels.
- Existing BIR validation already rejects stale present ids for direct calls,
  global loads/stores, pointer symbol values, and initializer symbols. The
  remaining gap is requiring ids on generated metadata-rich direct-symbol
  references, not replacing every display spelling.

## Proof

Source-level audit only, per delegated packet. No build/tests run and no
`test_after.log` required.

Local check passed: `git diff --check -- todo.md`.
