# Prepared Value-Location And Move Consumption For X86

Status: Closed
Created: 2026-04-18
Last-Updated: 2026-04-20
Closed: 2026-04-20
Disposition: Completed; prepared value-home and move-bundle lookups are now authoritative for the covered x86 consumer routes, and the remaining broader backend failures stay tracked as adjacent debt under existing follow-on ideas.
Depends-On:
- idea 58 shared CFG, join, and loop materialization
Blocks:
- idea 59 generic scalar instruction selection for x86

## Why This Was Closed

Idea 60 was about making prepared value-home and move-bundle facts the
authoritative handoff into the x86 prepared emitter. That route is now
complete for the intended scope: shared prepare publishes direct consumer
lookups, the x86 prepared consumer uses those lookups for the bounded scalar,
join, call, result, and return lanes covered by the runbook, and the latest
focused proof keeps missing-bundle handling on the shared contract instead of
reopening x86-local ABI or home guessing.

## Validation At Closure

Closure used the following validation:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result:

- focused handoff proof passed: `backend_x86_handoff_boundary`
- broader backend regression guard passed in non-decreasing mode
- before reported `68` passed / `4` failed / `72` total
- after reported `68` passed / `4` failed / `72` total
- no new backend failures were introduced at closure
- the remaining broader backend failures stayed limited to:
  - `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
  - `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`
  - `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`

## Follow-On Context

- `ideas/open/57_x86_backend_c_testsuite_capability_families.md` remains the
  follow-on route for the residual broader backend capability debt
- `ideas/open/59_generic_scalar_instruction_selection_for_x86.md` and
  `ideas/open/61_stack_frame_and_addressing_consumption.md` remain separate
  follow-on ideas and should not be folded back into this closed route

## Intent

This idea makes prepared value homes and move obligations authoritative.

If x86 receives a named value, it should be able to ask:

- where does this value live right now
- what register or stack slot is its assigned home
- what move obligations exist before a join, call, or return boundary

It should not need to guess those answers from ad hoc conventions or local
slot order.

## Concrete Data To Add

`PreparedRegalloc` already contains much of the raw data, but x86 needs a
consumer-oriented view instead of digging through multiple arrays.

Suggested additions in `src/backend/prealloc/prealloc.hpp`:

```cpp
enum class PreparedValueHomeKind {
  None,
  Register,
  StackSlot,
  RematerializableImmediate,
};

struct PreparedValueHome {
  PreparedValueId value_id = 0;
  std::string function_name;
  std::string value_name;
  PreparedValueHomeKind kind = PreparedValueHomeKind::None;
  std::optional<std::string> register_name;
  std::optional<PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> offset_bytes;
};

enum class PreparedMovePhase {
  BlockEntry,
  BeforeInstruction,
  BeforeCall,
  AfterCall,
  BeforeReturn,
};

struct PreparedMoveBundle {
  std::string function_name;
  PreparedMovePhase phase = PreparedMovePhase::BeforeInstruction;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::vector<PreparedMoveResolution> moves;
};

struct PreparedValueLocationFunction {
  std::string function_name;
  std::vector<PreparedValueHome> value_homes;
  std::vector<PreparedMoveBundle> move_bundles;
};

struct PreparedValueLocations {
  std::vector<PreparedValueLocationFunction> functions;
};
```

Then `PreparedBirModule` should carry the consumer view directly:

```cpp
struct PreparedBirModule {
  bir::Module module;
  PreparedControlFlow control_flow;
  PreparedValueLocations value_locations;
  PreparedStackLayout stack_layout;
  PreparedRegalloc regalloc;
};
```

The exact layout may vary, but x86 must get a direct lookup surface for value
homes and move bundles.

## Concrete Functions To Exist

Shared producer helpers:

```cpp
PreparedValueLocations build_prepared_value_locations(const PreparedRegalloc& regalloc,
                                                      const PreparedStackLayout& layout);

PreparedValueHome classify_prepared_value_home(const PreparedRegallocValue& value,
                                               const PreparedStackLayout& layout);

std::vector<PreparedMoveBundle> group_move_resolution_by_phase(
    const PreparedRegallocFunction& function_regalloc);
```

Consumer helpers:

```cpp
const PreparedValueLocationFunction* find_prepared_value_locations(
    const PreparedBirModule& module,
    std::string_view function_name);

const PreparedValueHome* find_prepared_value_home(
    const PreparedValueLocationFunction& function_locs,
    std::string_view value_name);

std::span<const PreparedMoveResolution> moves_before_call(
    const PreparedValueLocationFunction& function_locs,
    std::size_t block_index,
    std::size_t instruction_index);

std::span<const PreparedMoveResolution> moves_before_return(
    const PreparedValueLocationFunction& function_locs,
    std::size_t block_index,
    std::size_t instruction_index);
```

These helpers belong in shared `prepare` ownership, not x86-local ad hoc code.

## Producer / Consumer Boundary

### Produced By Prealloc / Regalloc

- value id to assigned register
- value id to assigned stack slot
- rematerializable values
- grouped move plans for joins, calls, and returns

### Consumed By X86

- operand sourcing for scalar selection
- call argument/result movement
- join-edge and block-entry movement
- return value placement

### Must Not Be Re-Derived In X86

- "small i32 values default to eax"
- "this local probably lives in the same stack slot as its source"
- "this call argument probably needs to move into abi register X"

## Code Examples

### Example Value Homes

```cpp
PreparedValueHome{
  .value_id = 7,
  .function_name = "main",
  .value_name = "%sum",
  .kind = PreparedValueHomeKind::Register,
  .register_name = "eax",
};

PreparedValueHome{
  .value_id = 8,
  .function_name = "main",
  .value_name = "%spill.tmp",
  .kind = PreparedValueHomeKind::StackSlot,
  .slot_id = 3,
  .offset_bytes = 24,
};
```

### Example Move Bundle

```cpp
PreparedMoveBundle{
  .function_name = "main",
  .phase = PreparedMovePhase::BeforeCall,
  .block_index = 0,
  .instruction_index = 5,
  .moves = {
    {.from_value_id = 7,
     .to_value_id = 0,
     .destination_kind = PreparedMoveDestinationKind::CallArgumentAbi,
     .destination_storage_kind = PreparedMoveStorageKind::Register,
     .destination_abi_index = 0,
     .destination_register_name = "edi"},
  },
};
```

With this handoff, x86 does not infer call moves by re-reading ABI metadata and
guessing current homes. It consumes the prepared bundle.

## Acceptance Shape

This idea is satisfied when x86 operand selection and call/return lowering can
query canonical value homes and move bundles directly, and emitter-local home
reconstruction is no longer necessary.

## Non-Goals

- stack-frame size computation
- memory-address provenance
- branch/join semantics
