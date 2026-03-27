# `stmt_emitter.cpp` Compression Refactor Plan

Target file: `src/codegen/lir/stmt_emitter.cpp`  
Current size: 4155 lines  
Primary goal: compress statement lowering by extracting repeated emission patterns, terminator management, aggregate access helpers, and call/assignment lowering scaffolding.

## Why This File

`stmt_emitter.cpp` is carrying too many responsibilities in one place:
- block/terminator management
- temporary naming
- global/object selection
- statement lowering
- expression lowering support
- aggregate/field access lowering
- call lowering
- inline asm glue

The top of file already shows good helper extraction; the rest of the file should follow the same pattern.

## Refactor Objective

Turn large statement/emission routines into coordinators over small, named helper families so that future changes can target one lowering concern at a time.

## Highest-Value Compression Targets

### 1. Terminator and block lifecycle helpers

Hot regions:
- label creation
- branch / condbr / ret / switch / unreachable emission
- repeated "if current block is still unterminated" checks

Plan:
- consolidate block-finalization policy into one small helper layer
- expose explicit helpers for:
  - `ensure_open_block(...)`
  - `set_terminator_if_open(...)`
  - `emit_fallthrough_br_if_needed(...)`

Goal:
- reduce repeated terminator-guard boilerplate

### 2. Aggregate/member/index/address lowering

Likely repeated motifs:
- field slot lookup
- address materialization before store/load
- GEP-ish access emission
- array vs struct vs pointer object access branching

Extract helpers such as:
- `emit_address_of_expr(...)`
- `emit_object_member_ptr(...)`
- `emit_struct_field_access(...)`
- `emit_array_element_address(...)`
- `emit_load_if_lvalue(...)`

Goal:
- centralize object-access policy

### 3. Call lowering

Likely repeated motifs:
- normal call vs indirect call vs builtin-ish call
- argument preparation
- varargs handling
- function pointer signature lookup

Extract helpers such as:
- `prepare_call_args(...)`
- `emit_direct_call(...)`
- `emit_indirect_call(...)`
- `resolve_call_signature(...)`
- `coerce_arg_for_call(...)`

Goal:
- keep call lowering branches short and explicitly named

### 4. Assignment/store family

Likely repeated motifs:
- lower RHS
- compute destination address
- handle aggregate copies / scalar stores / bitfield-ish cases

Extract helpers such as:
- `emit_store_to_lvalue(...)`
- `emit_scalar_assign(...)`
- `emit_aggregate_assign(...)`
- `emit_init_from_constant_or_expr(...)`

Goal:
- separate "what kind of assignment" from "where to store it"

### 5. Statement dispatcher compression

Hot region:
- whichever large `emit_stmt` / `lower_stmt` style dispatcher exists deeper in file

Plan:
- extract branch families by statement kind:
  - control flow
  - declarations
  - returns
  - labels/goto/switch
  - asm

Goal:
- top-level dispatcher should read like a table of cases, not a monolith

## Suggested Agent Work Packages

### Package A: terminator/block policy

Scope:
- block lifecycle helpers only

Tasks:
- extract and reuse termination checks
- reduce duplicated block-end handling

Validation:
- branch/switch/return lowering tests

### Package B: object access and store helpers

Scope:
- field/index/address/store support code

Tasks:
- centralize lvalue address formation
- separate scalar vs aggregate stores

Validation:
- struct field, array indexing, assignment, memcpy-like cases

### Package C: call lowering family

Scope:
- direct/indirect/builtin-ish calls

Tasks:
- factor arg preparation and signature resolution
- keep external declaration registration behavior unchanged

Validation:
- function pointer, variadic, builtin call, and extern-call tests

## Guardrails

- LIR emission is behaviorally brittle: no mixed "cleanup + semantic fix" patches.
- Preserve emitted IR shape when possible; if shape changes, verify with focused diff tests.
- Keep helper extraction local before moving code into new files.
- Add comments only where the control-flow contract is non-obvious.

## Good First Patch

Refactor the deepest statement dispatcher into per-statement-kind helpers while leaving existing low-level emission helpers untouched. That yields strong readability gains with manageable regression risk.
