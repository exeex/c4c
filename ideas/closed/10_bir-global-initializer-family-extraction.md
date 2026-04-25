# BIR Global Initializer Family Extraction

## Closure

Closed after extracting the global initializer parsing/lowering family into
`src/backend/bir/lir_to_bir/global_initializers.cpp`, preserving
`globals.cpp` ownership of global entry behavior and global address metadata.
Close proof used `c4c_codegen` plus the backend CTest subset
`ctest --test-dir build -j --output-on-failure -R '^backend_'`, with
regression guard passing at 97 passed, 0 failed before and after.

## Intent

Reduce `src/backend/bir/lir_to_bir/globals.cpp` pressure by extracting global
initializer parsing/lowering into an implementation-only file, without
changing global lowering semantics or adding headers.

This continues the BIR structural cleanup pattern: one semantic family per
small idea, `lowering.hpp` remains the private index, and implementation files
own coherent behavior without creating per-family headers.

## Rationale

`globals.cpp` currently owns several distinct global-lowering families:

```text
global scalar type parsing
integer array type parsing
LLVM byte string initializer lowering
global symbol/address/GEP initializer parsing
scalar initializer lowering
integer array initializer lowering
aggregate initializer lowering
known global address resolution
minimal global entry lowering
string constant global lowering
```

The initializer family is the clearest first extraction. It is large enough to
reduce `globals.cpp` pressure, but bounded enough to move mechanically and
prove with existing global/BIR tests.

## Design Direction

Keep ownership simple:

```text
globals.cpp
  owns global entry behavior and global address metadata behavior

global_initializers.cpp
  owns global initializer parsing and lowering helpers

lowering.hpp
  remains the complete private index for lir_to_bir_detail declarations
```

The new file should be an implementation split only. Do not create
`global_initializers.hpp`, `globals.hpp`, or other new headers.

## Scope

1. Create a new implementation file:

```text
src/backend/bir/lir_to_bir/global_initializers.cpp
```

2. Move only initializer parsing/lowering helpers from `globals.cpp`.
   Good candidates include:
   - `lower_llvm_byte_string_initializer`
   - `parse_global_symbol_initializer`
   - `parse_global_gep_initializer`
   - `parse_global_address_initializer`
   - `lower_global_initializer`
   - `lower_integer_array_initializer`
   - `lower_aggregate_initializer`
   - small helper structs/functions used only by these initializer routines

3. Keep global entry behavior in `globals.cpp`.
   - `lower_minimal_global` stays in `globals.cpp` unless moving a tiny wrapper
     is unavoidable.
   - `lower_string_constant_global` stays in `globals.cpp`.
   - `resolve_known_global_address` stays in `globals.cpp`.
   - global type parsing may stay in `globals.cpp` unless it is exclusively
     tied to the moved initializer helpers.

4. Preserve declarations and layering.
   - `lowering.hpp` remains the private declaration index.
   - Existing `lir_to_bir_detail` declarations remain reachable without adding
     a header.
   - Do not convert initializer helpers into free functions over a whole
     lowerer object; they should stay argument-driven where they already are.

5. Build and prove no behavior change.
   - Build `c4c_codegen`.
   - Run relevant BIR/LIR-to-BIR global initializer tests covering scalar
     globals, string/byte initializers, aggregate initializers, pointer/global
     address initializers, and GEP initializers.
   - Do not rewrite expectations.

## Acceptance Criteria

- `src/backend/bir/lir_to_bir/global_initializers.cpp` exists and owns global
  initializer parsing/lowering helpers.
- `globals.cpp` remains the owner of global entry behavior and global address
  metadata behavior.
- No new `.hpp` files are added.
- `lowering.hpp` remains the complete private lowerer/detail index.
- Global initializer, global address, string constant, and minimal global
  behavior are unchanged.
- `c4c_codegen` builds.
- Relevant global initializer tests pass with no expectation rewrites.

## Non-Goals

- Do not redesign global initializer semantics.
- Do not split all global lowering in one pass.
- Do not move string constant global lowering unless the active plan proves it
  is required for the initializer extraction.
- Do not introduce a global-lowering state owner.
- Do not add per-family headers.
