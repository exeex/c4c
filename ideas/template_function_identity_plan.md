# Template Function Identity Plan

## Goal

Make template function specialization selection use structured identity.

Do not use `mangled_name` as the primary semantic key anymore.

After this refactor:

- primary template family identity = `const Node* primary_def`
- selected specialization pattern identity = `const Node* selected_pattern`
- concrete instantiation identity = `primary_def + SpecializationKey`
- `mangled_name` = derived output only


## Read This First

This document is written for an execution-oriented agent.

Do not invent a new architecture.
Do not redesign the whole template pipeline.
Copy the shape that already exists for template structs and apply the same
pattern to template functions.

Primary references to copy from:

- [structured_template_identity_plan.md](/workspaces/c4c/ideas/structured_template_identity_plan.md)
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
  Current struct-side registry/key patterns and current function registry live
  here.
- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
  Current function-template registration, specialization collection, and
  lowering decisions live here.


## Current Problem

Function-template identity has been migrated to structured owner-first
selection.

Today the codebase has:

- good part:
  `TypeBindings`, `NttpBindings`, and `SpecializationKey` already exist
- also good:
  structured function-template identity scaffolding already exists in
  `compile_time_engine.hpp`
- remaining bad part:
  this document may lag behind the implementation if not updated after each
  cleanup step

The old path used to look like this:

1. register explicit specialization in
   [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
   by calling `mangle_specialization(...)`
2. store it in
   [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
   as `specs_[mangled]`
3. later, when lowering an instance, do
   `registry_.find_specialization(inst.mangled_name)`
4. if found, lower that body

That path has now been removed from the active function-template control flow.


## Non-Goals

Do not do these things in this plan:

- do not move deferred NTTP default evaluation into the engine
- do not redesign parser template syntax handling
- do not remove mangled names from emitted HIR or metadata
- do not refactor template struct identity again
- do not solve every template-function bug in one diff


## Desired End State

At the end of this plan:

- explicit specialization registration is owner-based, not mangled-name-based
- specialization selection starts from the primary template function node
- the result of specialization selection is structured
- dedup/instance identity uses `primary_def + spec_key`
- mangled names are still produced, but only after the semantic instance is
  known


## Current Status

As of 2026-03-24, the following pieces are already landed:

- `FunctionTemplateEnv`
- `SelectedFunctionTemplatePattern`
- `FunctionTemplateInstanceKey`
- owner-based function specialization storage
- structured specialization selection helper
- initial lowering path uses structured selection
- deferred template instantiation callback uses structured selection

Current follow-up:

- keep this document aligned with the code after each adjacent template change


## Files To Change

You are expected to modify these files:

1. [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- add structured function-template identity types
- change `InstantiationRegistry`
- remove string-only specialization registry from the main path

2. [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- change explicit specialization registration
- change specialization selection during lowering
- change deferred template function instantiation callback to use structured
  selection

You may also need small metadata touchups in:

3. [ir.hpp](/workspaces/c4c/src/frontend/hir/ir.hpp)
- only if you need to expose structured metadata in HIR debug dumps

Do not start in parser files for this task.


## Existing Code You Must Replace

These old pieces were the ones to remove from the main path.

In [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp):

- `specs_`
- `find_specialization(const std::string& mangled)`
- `register_specialization(const std::string& mangled, const Node* spec_node)`

In [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp):

- explicit specialization registration via `mangle_specialization(...)`
- lowering-time lookup via `registry_.find_specialization(inst.mangled_name)`
- deferred instantiation lookup via `registry_.find_specialization(mangled)`

These are now historical reference points for what was removed.


## Data Model

These concepts should exist in
[compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp).

They are already the right shape and should remain close to this:

```cpp
struct FunctionTemplateEnv {
  const Node* primary_def = nullptr;
  const std::vector<const Node*>* specialization_patterns = nullptr;
};

struct SelectedFunctionTemplatePattern {
  const Node* primary_def = nullptr;
  const Node* selected_pattern = nullptr;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SpecializationKey spec_key;
};

struct FunctionTemplateInstanceKey {
  const Node* primary_def = nullptr;
  SpecializationKey spec_key;
};
```

Do not over-design this.
Copy the template-struct identity pattern as closely as possible.


## Registry Changes Required

Update `InstantiationRegistry` in
[compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp).

### 1. Add owner-based specialization storage

Status:
- done

Replace the effective meaning of:

- `mangled specialization string -> specialization node`

with:

- `primary function template node -> list of specialization pattern nodes`

Suggested storage:

```cpp
std::unordered_map<const Node*, std::vector<const Node*>> function_specializations_;
```

Do not reintroduce `specs_` as a semantic lookup path.


### 2. Add structured instance keys

Status:
- done

Old dedup was effectively:

- `fn_name + "::" + mangled_name`

Change the semantic dedup key to:

- `primary_def + spec_key`

You can still keep `mangled_name` inside `TemplateSeedWorkItem` and
`TemplateInstance`, because codegen and HIR names still need it.

But semantic dedup must no longer depend only on the string.

Suggested storage:

```cpp
std::unordered_set<FunctionTemplateInstanceKey, ...> instance_keys_;
std::unordered_set<FunctionTemplateInstanceKey, ...> seed_keys_;
```

If using `unordered_set` for a custom key is annoying, use a stable string
wrapper built from:

- primary node address
- `SpecializationKey`

That is acceptable as a temporary implementation detail.
The important part is that the semantic identity starts from `primary_def`,
not from `mangled_name`.


### 3. Add owner-based specialization selection helpers

Status:
- done

Add helper APIs like:

```cpp
FunctionTemplateEnv build_function_template_env(const Node* primary_def) const;

SelectedFunctionTemplatePattern select_function_specialization_pattern(
    const Node* primary_def,
    const TypeBindings& bindings,
    const NttpBindings& nttp_bindings) const;
```

These helpers should:

1. start from the primary template definition
2. inspect registered explicit specializations under that primary
3. compare specialization args against concrete bindings
4. return the selected pattern, or the primary if no specialization matches


## Selection Logic To Reuse

Do not invent function-specific matching rules if you can avoid it.

Use the existing ingredients already in the codebase:

- `SpecializationKey`
- `TypeBindings`
- `NttpBindings`
- explicit specialization AST data on `Node`
  - `template_arg_types`
  - `template_arg_values`
  - `template_arg_is_value`

The main path should compare semantic bindings, not reproduced mangled
strings.


## AST To HIR Changes Required

Update
[ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp).

### 1. Registration pass

Current code:

- collects template defs with `ct_state_->register_template_def(...)`
- registers explicit specializations under the primary definition

Change it to:

- find the primary function template definition
- register the specialization under that primary definition
- do not make mangled-string lookup the only source of truth


### 2. Lowering of discovered instances

Current code:

```cpp
const Node* spec_node = registry_.find_specialization(inst.mangled_name);
if (spec_node) {
  lower_function(spec_node, &inst.mangled_name);
} else {
  lower_function(item, &inst.mangled_name, ...);
}
```

Status:
- done for the primary lowering path

Replace it with:

1. derive the selected pattern from `primary_def + bindings + nttp_bindings`
2. if selected pattern is an explicit specialization, lower that node
3. otherwise lower the primary template body with bindings

The lowering decision must not start from `inst.mangled_name`.


### 3. Deferred template instantiation callback

Status:
- done for the deferred callback path

Change it the same way:

1. find `fn_def`
2. run structured selection from `fn_def + bindings + nttp_bindings`
3. lower the selected pattern


## Concrete Removal Checklist

This is the minimum checklist for the completed cutover.

In [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp):

- keep `FunctionTemplateEnv`
- keep `SelectedFunctionTemplatePattern`
- keep `FunctionTemplateInstanceKey`
- keep owner-based specialization registry as the primary registry
- keep owner-based specialization lookup/select helper as the primary selector
- stop treating `specs_` as the primary specialization registry
- stop treating `fn_name + "::" + mangled` as the semantic instance key

In [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp):

- stop registering explicit specializations only by mangled string
- stop selecting function specializations by `inst.mangled_name`
- stop selecting deferred function specializations by `mangled`
- continue producing mangled names only as derived output for lowered HIR


## Implementation Order

Do the work in this order.
Do not jump around.

### Step 1. Add structured helper types

File:
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

Status:
- already complete

Add:
- `FunctionTemplateEnv`
- `SelectedFunctionTemplatePattern`
- `FunctionTemplateInstanceKey`


### Step 2. Add owner-based specialization registry

File:
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

Status:
- already complete

Add:
- register specializations by `primary_def`
- query specialization list by `primary_def`


### Step 3. Add structured specialization selection helper

Files:
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp) if helper bodies fit better there

Status:
- already complete

Implement:
- select specialization from `primary_def + bindings + nttp_bindings`

Reference design:
- the template-struct owner-first selection path introduced by
  [structured_template_identity_plan.md](/workspaces/c4c/ideas/structured_template_identity_plan.md)


### Step 4. Switch lowering over to structured selection

File:
- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Status:
- already complete

Replace both:
- initial lowering path for discovered instances
- deferred instantiation callback path


### Step 5. Switch semantic dedup to structured key

File:
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

Status:
- complete

Change:
- seed dedup
- realized-instance dedup

Keep:
- `mangled_name` as stored output metadata


### Step 6. Remove old path from the main control flow

Files:
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Status:
- complete

Not allowed:
- reintroduce old mangled lookup as specialization-selection control flow


## How To Match Explicit Specializations

Use this simple rule:

1. start from the primary template function
2. iterate its explicit specialization nodes
3. for each specialization, compare its declared template args to the concrete
   bindings for this instantiation
4. if all declared args match, choose that specialization
5. if none match, choose the primary template body

Do not add partial ordering.
Do not add SFINAE.
Do not add a new ranking system.

This plan only needs the same level of matching the codebase already supports.


## Design Reference To Copy

When unsure, copy the style of the template-struct refactor:

- owner-first registration
- structured selection result
- structured instance key for dedup
- mangled name retained as derived output

Primary design reference:
- [structured_template_identity_plan.md](/workspaces/c4c/ideas/structured_template_identity_plan.md)

Primary implementation references:
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)


## Acceptance Criteria

This plan is complete when all of the following are true:

1. explicit function-template specialization selection does not primarily use
   `mangled_name`
2. owner-based specialization registration exists
3. lowering selects specialization bodies from `primary_def + bindings +
   nttp_bindings`
4. semantic dedup for template function instances is based on
   `primary_def + spec_key`
5. mangled names remain only derived/codegen/debug data


## Immediate Next Work

If continuing from the current tree, focus on adjacent template behavior:

1. keep the targeted regression set green while working on lazy instantiation
2. investigate `eastl_type_traits_signed_helper_base_expr_parse`
3. continue `template_lazy_instantiation_plan.md`


## Testcases To Run

At minimum, run these targeted tests after the change:

Core specialization / identity:

- `cpp_positive_sema_specialization_identity_cpp`
- `cpp_hir_specialization_key`
- `cpp_hir_template_call_info_dump`

Template function defaults / deduction / NTTP:

- `cpp_positive_sema_template_arg_deduction_cpp`
- `cpp_positive_sema_template_default_args_cpp`
- `cpp_positive_sema_template_nttp_cpp`
- `cpp_positive_sema_template_bool_nttp_cpp`
- `cpp_positive_sema_template_char_nttp_cpp`
- `cpp_positive_sema_template_integral_nttp_types_cpp`
- `cpp_positive_sema_template_nttp_forwarding_depth_cpp`
- `cpp_positive_sema_template_recursive_nttp_cpp`

Consteval + template-function interaction:

- `cpp_positive_sema_consteval_template_cpp`
- `cpp_positive_sema_consteval_nested_template_cpp`
- `cpp_positive_sema_consteval_template_sizeof_cpp`
- `cpp_hir_consteval_template_dump`
- `cpp_hir_consteval_template_call_info_dump`
- `cpp_hir_consteval_template_reduction_verified`

Regression guards around nearby template behavior:

- `cpp_positive_sema_template_alias_deferred_nttp_expr_runtime_cpp`
- `cpp_positive_sema_eastl_slice6_template_defaults_and_refqual_cpp`


## Known Out Of Scope Failure

This plan does not need to fix:

- `eastl_type_traits_signed_helper_base_expr_parse`

That case is adjacent template behavior and is better tracked separately from
the function-template identity cutover.


## Short Version

If you are implementing this plan, do exactly this:

1. in [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp),
   add owner-based function specialization registry and structured instance keys
2. in [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp),
   stop selecting explicit specializations by `inst.mangled_name`
3. select from `primary_def + bindings + nttp_bindings` instead
4. keep `mangled_name` only as derived output for the lowered HIR function name
5. run the listed template-function tests and make sure they still pass
