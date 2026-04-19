# Stack Frame And Addressing Consumption For Prepared X86

Status: Open
Created: 2026-04-18
Last-Updated: 2026-04-18
Depends-On:
- idea 58 shared CFG, join, and loop materialization
Blocks:
- idea 59 generic scalar instruction selection for x86

## Deactivation Note

2026-04-19: active execution switched away from this idea before Step 3.2 was
exhausted. The current route has already moved direct frame-slot and same-module
global guard/helper consumers toward authoritative prepared addressing, but
string-backed and residual direct symbol lanes still remain on the open side of
Step 3.2. Resume from the active `plan.md` / `todo.md` history for idea 61 if
this idea is reactivated later.

## Intent

This idea makes frame layout and memory-address provenance canonical for the
prepared x86 route.

The x86 emitter should not rebuild:

- local-slot offsets
- frame size
- stack-relative addressing expressions
- whether a memory access is direct-frame, global, or pointer-indirect

Those facts should arrive from shared prepared data.

## Concrete Data To Add

`bir::MemoryAddress` is still too semantic for x86 consumption by itself.
The prepared handoff needs a consumer-oriented addressing map.

Suggested additions in `src/backend/prealloc/prealloc.hpp`:

```cpp
enum class PreparedAddressBaseKind {
  None,
  FrameSlot,
  GlobalSymbol,
  PointerValue,
  StringConstant,
};

struct PreparedAddress {
  PreparedAddressBaseKind base_kind = PreparedAddressBaseKind::None;
  std::optional<PreparedFrameSlotId> frame_slot_id;
  std::optional<std::string> symbol_name;
  std::optional<std::string> pointer_value_name;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool can_use_base_plus_offset = false;
};

struct PreparedMemoryAccess {
  std::string function_name;
  std::string block_label;
  std::size_t inst_index = 0;
  std::optional<std::string> result_value_name;
  std::optional<std::string> stored_value_name;
  PreparedAddress address;
};

struct PreparedAddressingFunction {
  std::string function_name;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  std::vector<PreparedMemoryAccess> accesses;
};

struct PreparedAddressing {
  std::vector<PreparedAddressingFunction> functions;
};
```

Then `PreparedBirModule` should expose:

```cpp
struct PreparedBirModule {
  bir::Module module;
  PreparedStackLayout stack_layout;
  PreparedAddressing addressing;
};
```

The exact storage shape may differ, but x86 needs direct access to frame and
address meaning.

## Concrete Functions To Exist

Producer helpers:

```cpp
PreparedAddressing build_prepared_addressing(const bir::Module& module,
                                             const PreparedStackLayout& layout);

PreparedAddress classify_prepared_address(const bir::MemoryAddress& address,
                                          const PreparedStackLayout& layout,
                                          std::string_view function_name);

PreparedMemoryAccess build_prepared_memory_access(const bir::Function& function,
                                                  const bir::Block& block,
                                                  std::size_t inst_index,
                                                  const bir::Inst& inst,
                                                  const PreparedStackLayout& layout);
```

Consumer helpers:

```cpp
const PreparedAddressingFunction* find_prepared_addressing(
    const PreparedBirModule& module,
    std::string_view function_name);

const PreparedMemoryAccess* find_prepared_memory_access(
    const PreparedAddressingFunction& function_addr,
    std::string_view block_label,
    std::size_t inst_index);

std::string render_frame_slot_operand(const PreparedAddress& address,
                                      const PreparedStackLayout& layout,
                                      std::string_view size_name);
```

## Producer / Consumer Boundary

### Produced By Shared Prepare

- frame size and alignment
- frame slot identity
- direct frame-slot access vs pointer-indirect access
- symbol-backed addressing vs stack-backed addressing
- base-plus-offset legality

### Consumed By X86

- prologue and epilogue stack adjustment
- local/global/string memory operand emission
- load/store operand spelling

### Must Not Be Re-Derived In X86

- `build_local_slot_layout()`
- stack offset from `function.local_slots` iteration order
- address provenance from stringly slot-name conventions

## Code Examples

### Example Prepared Memory Access

```cpp
PreparedMemoryAccess{
  .function_name = "main",
  .block_label = "entry",
  .inst_index = 3,
  .result_value_name = "%tmp",
  .address = PreparedAddress{
    .base_kind = PreparedAddressBaseKind::FrameSlot,
    .frame_slot_id = 2,
    .byte_offset = 8,
    .size_bytes = 4,
    .align_bytes = 4,
    .can_use_base_plus_offset = true,
  },
};
```

X86 can now emit:

```cpp
mov eax, DWORD PTR [rsp + 24]
```

because the frame slot and offset are already canonical.

### Example Pointer-Indirection

```cpp
PreparedAddress{
  .base_kind = PreparedAddressBaseKind::PointerValue,
  .pointer_value_name = "%p",
  .byte_offset = 16,
  .size_bytes = 8,
  .align_bytes = 8,
  .can_use_base_plus_offset = true,
};
```

This tells x86 it must first source `%p` from the value-location contract and
then form the memory operand from that pointer, rather than pretending the
access is a direct frame slot.

## Acceptance Shape

This idea is satisfied when x86 can emit prologue/epilogue and load/store
addressing from canonical prepared frame and address data, and the emitter no
longer rebuilds local slot layout privately.

## Non-Goals

- choosing arithmetic opcodes
- assigning registers
- join or branch semantics
