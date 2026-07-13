# Focused LIR Carrier Contract

This document records the plan-Step-4 contract for the four focused probes
bound in `authority_matrix.md` and its landed Step 5 status at HEAD
`e7a24c93d`. The carrier and four bounded producer/verifier contracts are now
implemented. This remains explicitly not a claim of new-BIR receipt.

## Selected carrier

Extend the existing `LirOperand` rather than adding operation-specific ID
mirrors. Its spelling remains available to the printer, while one closed
authority alternative supplies semantic identity:

```cpp
struct LirIntegerImmediate {
  long long value;
};

using LirOperandAuthority =
    std::variant<std::monostate, LirValueId, LinkNameId,
                 LirIntegerImmediate>;

struct LirOperand {
  std::string display;
  LirOperandKind kind;
  LirOperandAuthority authority;

  static LirOperand ssa(std::string display, LirValueId id);
  static LirOperand global(std::string display, LinkNameId id);
  static LirOperand integer(std::string display, long long value);
};
```

`str()` and stored text are presentation only. Semantic equality, ownership, and
consumer receipt use `authority`. Integer signedness and width are contextual:
the immediate payload is paired with the owning operation's `LirTypeRef` (or a
GEP index fact's `LirTypeRef`). A `monostate` preserves a phased migration path
outside the focused authoritative producer shapes. Direct-global store/load
and authoritative GEP shapes reject missing authority. `LirRet` deliberately
retains raw non-void compatibility for unowned producers while ordinary scalar
integer producers populate authority.

The authoritative factories set the stored `LirOperandKind` from their role:
`ssa` sets `SsaValue`, `global` sets `Global`, and `integer` sets `Immediate`.
They do not call `classify(display)`. Verification compares the authority
alternative with that stored kind and the operation field role; it never
classifies or parses display. Legacy compatibility construction may still
classify its presentation string while carrying `monostate`, but that path is
not accepted in direct-global store/load or authoritative GEP shapes. Raw
return compatibility remains outside the authoritative scalar return shape.

The foundation packet moved—not copied—the single `LirValueId` definition into
`identity.hpp`, included by both `operands.hpp` and `ir.hpp`. `ir.hpp`
continues to re-export that same type. No nested integer, alias mirror, or
second `LirValueId` definition was introduced.

The selected design deliberately has no `result_id`, `ptr_link_name_id`, or
`immediate_value` mirror beside an operand. Such mirrors would create a second
source of truth. Compatibility text may remain temporarily only when derived
from the structured fact and ignored by semantic validation and import.

GEP indices use one additional generic typed-or-raw fact because the former
`vector<string>` combined type and value:

```cpp
class LirGepIndex {
  // typed(type, value) is authoritative; raw(presentation) is compatibility.
};
```

The focused GEP owns `vector<LirGepIndex> indices`; the printer renders typed
facts directly. The focused return fields retain aggregate-initializer names
`value_str` and `type_str`, but their actual types are
`optional<LirOperand>` and `LirTypeRef`. They are the only semantic fields, not
raw mirrors.

## Producer and allocation evidence

- `StmtEmitter::fresh_tmp` increments `FnCtx::tmp_idx` and returns a
  spelling such as `%t0`. Labels share that counter. It is therefore a display
  allocator, not stable value identity.
- `LirFunction::alloc_value()` is the existing value-ID convention. HIR
  lowering constructs the eventual function shell before statement/block
  emission and gives `FnCtx` access to that exact allocator; no parallel ID
  counter exists.
- `LirOperand fresh_value(FnCtx&)` calls the exact current function shell's
  `alloc_value()` for authority and,
  separately, `fresh_tmp(ctx)` for display before returning `ssa(display, id)`.
  It never derives an ID from `tmp_idx`. `fresh_lbl` and labels continue to use
  the presentation counter only.
- `LirBlockId` is already assigned structurally from block ownership. It is a
  useful precedent, but CFG fields are outside these focused contracts.
- HIR globals already carry `LinkNameId`; `lower_globals` preserves it into
  `LirGlobal`, and the LIR module preserves the link-name table.
  `select_global_object(DeclRef)` resolves the exact global, and the focused
  producer creates `LirOperand::global(display, gv.link_name_id)` at that seam.
- Assignment lowering creates native immediate authority from the original HIR
  `IntLiteral` while it is still available. Representation-preserving coercion
  rebuilds authority from that native payload; rendered spelling is never
  parsed.
- Global load and array-decay lowering select the exact `GlobalVar`, allocate
  the result from the current `LirFunction`, retain its `LinkNameId`, and build
  literal GEP indices from native values.
- Return lowering uses the function/expression `TypeSpec` plus the
  `emit_rval_operand` carrier before `emit_term_ret`. Same-representation
  scalar integers preserve immediate/SSA authority; emitted coercions remain
  raw compatibility, synthesized integer zero is native, and void returns are
  valueless.

### Source-time authoritative transport (landed)

The carrier survives the intermediate APIs; attaching an ID only at final
operation construction would have been too late. The foundation introduced
narrow operand-returning transport beside compatibility spelling access:

- `emit_lval_operand(...) -> LirOperand` carries a selected global from
  `emit_lval` into `AssignableLValue.ptr`, whose field becomes `LirOperand`.
  The focused global-lvalue branch constructs `global(display, link_name_id)`
  immediately after `select_global_object`. A compatibility `emit_lval(...)`
  wrapper may return `.str()` for callers not yet migrated.
- `emit_rval_operand(...) -> LirOperand` carries the HIR `IntLiteral` through
  the current `emit_rval_id -> std::string rhs` seam. The focused set-assignment
  route passes that same operand through `emit_set_assign_value` and
  `emit_store_assignable_value` into `LirStoreOp.val`; coercion must preserve or
  deliberately rebuild native authority from native source facts, never from
  the coerced spelling.
- The global-DeclRef coordinator branches for `LirLoadOp` and `LirGepOp` use
  `fresh_value(ctx)` and return that same result operand through
  `emit_rval_operand`. Compatibility callers may consume `.str()`, but the
  operation and authoritative callers retain the ID-bearing operand.

If changing these signatures touches broad call graphs, the first packet adds
the operand transport and compatibility wrappers, then migrates only the four
focused producer routes. Remaining callers may explicitly wrap their current
spelling in a `monostate` compatibility operand until their own checked row is
owned. No map keyed by strings, late registry, display scan, or reconstruction
from `emit_rval_id`/`emit_lval` output is permitted.

The compatibility split must also be recursion-safe and explicit: retain or
rename the old implementation as a text-only helper for unowned payloads;
`emit_rval_operand` and `emit_lval_operand` intercept the focused literal/global
branches and otherwise wrap that helper's result in `monostate`.
`emit_rval_id`/`emit_lval` become display-only wrappers over the operand route
for migrated entry points. Neither wrapper may be called to rediscover
authority after it has returned a string.

## Ownership rules

`LirValueId` is local to one `LirFunction`. Verification is two-pass within
each function: first collect every authoritative result definition and reject
invalid or duplicate IDs; then resolve every authoritative value use against
that function's definition set. Two functions may use the same numeric ID.
A use cannot borrow authority from another function even when its display is
identical. The two-pass rule permits forward references and PHI-like ordering
without weakening ownership.

`LinkNameId` is resolved through the containing `LirModule` link-name table.
For these focused global operands, it must also name exactly one corresponding
`LirGlobal` object in that module. An invalid or unresolved ID, an ID owned
only by a function or other non-global entity, an ID with no `LirGlobal` owner,
or ambiguous duplicate `LirGlobal` ownership is rejected. A valid ID owned by
a different `LirGlobal` simply denotes that different global; the verifier has
no separate display-intended object against which to call it “wrong.” It never
recovers or redirects an ID by comparing or parsing `@name` display text.

## Per-probe contracts

Each focused authority-matrix row binds to exactly one row here.

<a id="cc-store-1"></a>

### CC-STORE-1 — scalar global store

| Contract part | Requirement |
|---|---|
| Focused probe | `tests/backend/case/lir_identity_global_store.c` |
| Exact fields | `LirStoreOp.val`, `LirStoreOp.ptr`, and existing `LirStoreOp.type_str` |
| Carrier | `val = integer("7", 7)`; `ptr = global("@lir_identity_scalar", link_name_id)`; type remains authoritative `LirTypeRef(i32)` |
| Producer rule | `emit_rval_operand` creates the immediate from the HIR `IntLiteral`; `emit_lval_operand` stores the selected global operand in `AssignableLValue.ptr`; `emit_set_assign_value` and `emit_store_assignable_value` pass both operands unchanged to the store, except for native-source coercion that deliberately rebuilds authority |
| Reachable LIR verification | Require integer authority compatible with the store integer type; resolve the pointer `LinkNameId` to exactly one module global; reject missing/wrong carrier alternatives |
| Positive proof shape | The first body fact remains `store i32 7, ptr @lir_identity_scalar`, now with native value and symbol authority |
| Nearby positive coverage | A second integer payload including `0` or `-1`, and another internal or external scalar global selected through the same route |
| Negative coverage | Missing authority, symbol alternative on `val`, immediate alternative on `ptr`, invalid/unresolved or non-global/ownerless/ambiguous `LinkNameId`, and integer/type incompatibility; a misleading pointer display paired with another valid global ID must resolve to the ID-owned global |

<a id="cc-load-1"></a>

### CC-LOAD-1 — scalar global load

| Contract part | Requirement |
|---|---|
| Focused probe | `tests/backend/case/global_load.c` |
| Exact fields | `LirLoadOp.result`, `LirLoadOp.ptr`, and existing `LirLoadOp.type_str` |
| Carrier | `result = fresh_value(ctx)` (internally `ssa(fresh_tmp(ctx), current_fn.alloc_value())` with independently obtained display/ID); `ptr = global("@g_counter", link_name_id)`; type remains authoritative `LirTypeRef(i32)` |
| Producer rule | The global-DeclRef `emit_rval_operand` branch allocates the result through the current function shell, populates the pointer from the selected `GlobalVar`, constructs `LirLoadOp` with both, and returns the same result operand; compatibility callers may take only `.str()` |
| Reachable LIR verification | Register the result as one definition in the current function; reject invalid/duplicate IDs; resolve the global owner; require result/pointer carrier-kind coherence |
| Positive proof shape | The first body fact remains `%t0 = load i32, ptr @g_counter`, with display independent from result and symbol identity |
| Nearby positive coverage | A second load result in the same function and a load from another selected scalar global |
| Negative coverage | Duplicate result ID, invalid result ID, result using global authority, pointer using value authority, and invalid/unresolved or non-global/ownerless/ambiguous global ID; a misleading pointer display paired with another valid global ID still denotes that other global |

<a id="cc-gep-1"></a>

### CC-GEP-1 — global array address

| Contract part | Requirement |
|---|---|
| Focused probe | `tests/backend/case/lir_identity_global_array_address.c` |
| Exact fields | `LirGepOp.result`, `LirGepOp.ptr`, `LirGepOp.indices`, existing `element_type`, and `inbounds` |
| Carrier | Result is `fresh_value(ctx)`; base is the selected global operand; indices are two `LirGepIndex{LirTypeRef(i64), integer("0", 0)}` facts |
| Producer rule | The global-DeclRef `emit_rval_operand` branch allocates the result through the current function, retains the selected array global ID, constructs each native typed index, builds the GEP, and returns the same result operand; it never parses `"i64 0"` |
| Reachable LIR verification | Check definition ownership, global ownership, valid element/index types, nonempty structured indices, and index operands restricted to integer immediates or current-function SSA values |
| Positive proof shape | The first body fact remains `%t0 = getelementptr [1 x i32], ptr @lir_identity_array, i64 0, i64 0`, with no preceding cast |
| Nearby positive coverage | A structured SSA-valued integer index plus another array extent/base selected through the ordinary producer |
| Negative coverage | Empty/missing index facts, invalid index type, missing index authority, global authority used as an index, duplicate/invalid result ID, and invalid/unresolved or non-global/ownerless/ambiguous base ID; a misleading base display paired with another valid global ID still denotes that other global |

<a id="cc-ret-1"></a>

### CC-RET-1 — typed scalar return

| Contract part | Requirement |
|---|---|
| Focused probe | `tests/backend/case/aarch64_return_zero_smoke.c` |
| Exact fields | `LirRet.value_str` (`optional<LirOperand>`) and `LirRet.type_str` (`LirTypeRef`); spellings are compatibility names, not raw mirrors |
| Carrier | `value_str = integer("0", 0)` and `type_str = LirTypeRef(i32)` |
| Producer rule | `emit_rval_operand` retains native immediate/SSA facts; `stmt.cpp` passes a structured operand and type through the structured `emit_term_ret` API, preserving authority only across its no-instruction representation-preserving coercion path |
| Reachable LIR verification | Enforce void/value shape, valid type authority, immediate/type compatibility, and current-function ownership for SSA-valued returns |
| Positive proof shape | The first body fact remains `ret i32 0`, with no ordinary instruction required |
| Nearby positive coverage | A void return and a non-void return of a defined SSA result |
| Negative coverage | Non-void return without value, void return with value, invalid type, symbol-valued return, unknown/cross-function value ID, and incompatible immediate/type |

The ordinary focused store/load/GEP/return producer shapes now populate
authority, and authoritative shapes activate their complete verifier contract.
Other authority-matrix rows may retain `monostate`; that is phased migration,
not permission to create per-operation shadow IDs. Raw `LirRet` compatibility
is similarly outside the authoritative scalar producer shape.

## Rejection matrix

| Malformed or conflicting state | LIR verifier obligation | Why no text parsing is needed |
|---|---|---|
| Invalid `LirValueId` on a definition or use | Reject | ID validity is native |
| Duplicate definition ID in one function | Reject during definition collection | Compare IDs, not `%tN` spellings |
| Unknown or foreign-function value use | Reject during per-function use resolution | Resolve only in the current function's registry |
| Direct-global store/load or authoritative GEP operand has `monostate` | Reject | Inspect the closed authority variant; raw return and other unowned producers remain compatibility |
| Authority conflicts with operand kind/role | Reject: result/use requires value ID, global pointer requires `LinkNameId`, literal requires integer immediate | Match variant alternative to the field role and `LirOperandKind` |
| Integer immediate conflicts with contextual type | Reject non-integer/void type and values outside the representable contract chosen by implementation | Inspect native payload and `LirTypeRef` |
| Invalid or unresolved `LinkNameId` | Reject | Resolve the native ID in the module table |
| Link ID is owned only by a function/non-global entity, has no `LirGlobal` owner, or has ambiguous duplicate `LirGlobal` ownership | Reject | Compare the ID to structured global objects |
| Link ID names another valid `LirGlobal` while display spells the focused global | Accept the authority as a reference to the other global; a test must prove the ID wins | Display does not identify an intended object |
| GEP index lacks a type or value authority | Reject | Both members are structured |
| GEP index has non-integer type or symbol authority | Reject | Inspect type and authority alternatives |
| Non-void return lacks a value, or void return carries one | Reject | Inspect `LirTypeRef` and optional presence |
| Return value uses a symbol alternative | Reject | Match the carrier alternative to return-value roles |
| Display suggests a different value, symbol, or type | Do not reinterpret it and do not let it alter semantic resolution. Factories set kind from authority/role, and compatibility display is presentation only. Tests must prove misleading display cannot redirect identity. | There is intentionally no semantic authority-versus-display comparison |

The last row is the display conflict rule: the verifier must not parse `%t0`,
`@g`, `7`, or `i64 0` to recover or cross-check authority. Where canonical
printer spelling is desired, factories or printer tests enforce presentation;
canonical display parity is a producer/printer test, not verifier identity.
Semantic verification remains stable even if display is misleading.

## LIR verifier versus importer

The producer-side LIR verifier owns all checks available before import:

- carrier alternative and operand-kind/field-role coherence;
- valid and unique result definitions plus current-function use ownership;
- module link-name resolution and exact global-object ownership;
- native immediate and contextual `LirTypeRef` compatibility;
- structured GEP index shape; and
- void/non-void return shape.

The later LIR-to-BIR importer owns only consumer support: map an already
verified LIR value ID to the BIR definition/use registry, map an already
verified global ID to its BIR global receipt, and lower typed immediates and
operation semantics. It must not parse display, synthesize missing authority,
or duplicate producer ownership checks as a substitute for the LIR verifier.
Passing the LIR verifier therefore does not claim that a focused operation is
supported by the importer.

## Completed Step 5 packet order

1. **Generic foundation.** Relocate the one existing `LirValueId` definition
   into the shared LIR identity/model header; add the single operand authority
   variant and role-setting factories; construct the eventual `LirFunction`
   shell before emission; add `fresh_value(ctx)` backed by that shell's
   `alloc_value()` plus separate `fresh_tmp` display; add operand-returning
   lvalue/rvalue transport, explicit monostate compatibility wrappers, and
   per-function definition/use registry scaffolding. This packet does not
   claim any focused operation is importable.
2. **CC-STORE-1.** Migrate focused `AssignableLValue.ptr` and the
   `emit_rval_operand -> emit_set_assign_value -> emit_store_assignable_value`
   route so selected-global and native integer authority reaches the store;
   add focused and nearby positive/negative verifier coverage.
3. **CC-LOAD-1.** Allocate load results through the function owner and preserve
   selected-global authority; prove duplicate/invalid result and global-owner
   rejection.
4. **CC-GEP-1.** Introduce `LirGepIndex`, allocate the result, preserve the
   global base, and construct native typed indices. Progress beyond the focused
   GEP exposes the next operation; it is not whole-test capability.
5. **CC-RET-1.** Replace return text authority with the typed optional operand
   and `LirTypeRef`, populating it before the former string-loss seam. Prove
   immediate, SSA-valued, void, and malformed return shapes.

Packets may be combined only when they remain one coherent, independently
verified slice. No packet may claim success by adding a text parser, a
testcase-shaped producer, a parallel allocator, or an operation-specific
authority mirror.
