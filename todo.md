Status: Active
Source Idea Path: ideas/open/688_initializer_lowering_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Isolate String Constant Rewrite State

# Current Packet

## Just Finished

Step 3 completed the string-constant target-id rewrite isolation by moving the
rewrite helper out of module-local ownership and into the initializer/global
adapter path.

Exact files/helpers touched:
- `src/backend/bir/lir_to_bir/lowering.hpp`: declared
  `apply_string_pointer_initializer_target_ids(GlobalTypes&, LinkNameTable&)`
  alongside pointer-initializer resolution helpers.
- `src/backend/bir/lir_to_bir/globals.cpp`: added the adapter-owned helper that
  interns target ids for aggregate pointer-initializer addresses whose target is
  a string constant and whose address does not already carry a LinkNameId.
- `src/backend/bir/lir_to_bir/module.cpp`: removed the module-local helper and
  kept the same call ordering before relocation-slot/value-id publication.

Behavioral scope: no BIR output contract, relocation-slot publication,
prepared object-data behavior, string data layout, diagnostics, runtime
behavior, expectations, unsupported markers, tests, allowlists, or harness
policy were changed.

## Suggested Next

Next coherent packet: Step 4 should narrow initializer value materialization in
`src/backend/bir/lir_to_bir/global_initializers.cpp`, keeping scalar, byte
string, array, aggregate, and pointer initializer spelling compatibility behind
adapter-owned helpers without changing emitted initializer facts.

## Watchouts

- The rewrite still intentionally runs before
  `apply_resolved_pointer_initializer_value_ids()` so relocation slots and
  named pointer initializer values see the same string target ids as before.
- `GlobalInfo::is_string_constant` remains the existing adapter-private marker
  consumed by memory lowering; this packet did not widen or rename that shared
  state.
- Step 4 should avoid changing pointer initializer parsing semantics,
  relocation spelling, prepared object-data publication, or expectation files.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)'`

Result: passed. Build completed and 303 selected tests passed with 0 failures,
including `string_authority_guard`. Fresh proof output is preserved in
`test_after.log`.
