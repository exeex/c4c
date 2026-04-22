# `core/x86_codegen_types.hpp`

Primary role: declare the shared data carriers that multiple replacement files
need without forcing them through one mixed mega-header.

Owned inputs:

- common x86 codegen concepts that must be named across api, module, lowering,
  prepared, and debug boundaries
- stable state carried between lowering families and output helpers

Owned outputs:

- lightweight shared structs, enums, and aliases for output handles, target
  selections, symbol/render options, and canonical x86 state references
- type-level seams that let later headers depend on one another without
  dragging in unrelated helper declarations

Allowed indirect queries:

- `abi/x86_target_abi.hpp` types when shared structs need explicit ABI-facing
  fields
- forward declarations for lowering-owned services rather than full lowering
  helper declarations

Forbidden knowledge:

- public entrypoint overloads or API orchestration rules
- concrete text-emission helper bodies
- prepared dispatch contexts that expose broad hidden knowledge
- route-debug reporting formats

Role classification:

- `core`

Drafting notes:

- keep this file closer to a small vocabulary header than a service hub
- prefer narrow type ownership over helper accumulation so future files can
  include it without inheriting the old `x86_codegen.hpp` blast radius
