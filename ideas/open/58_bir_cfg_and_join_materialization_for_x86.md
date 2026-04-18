# Shared CFG, Branch, Join, And Loop Materialization Before X86 Emission

Status: Open
Created: 2026-04-18
Last-Updated: 2026-04-18
Depends-On: none
Blocks:
- idea 59 generic scalar instruction selection for x86

## Intent

This idea defines the prepared control-flow contract that x86 must consume
instead of reconstructing branch, join, short-circuit, and loop meaning from
whole-function CFG shape.

The key rule is simple:

- shared lowering decides what a branch condition means
- shared lowering decides what values cross each edge into a join
- x86 consumes those prepared decisions
- x86 does not rediscover them by matcher growth in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

## Problem

Today `PreparedControlFlow` already exists, but the contract is still too weak.
The current data can name a few branch predicates and some join transfers, but
it is not yet strong enough to cover ordinary branch lowering as a general
consumer contract.

That weakness shows up when x86 still needs to answer questions such as:

- is this condition a materialized bool or a fused compare-and-branch
- which predecessor owes which value to a join block
- is this join a select materialization, phi replacement, or loop-carried
  transfer
- which loop header values are carried by backedges

Those are shared lowering questions, not target spelling questions.

## Concrete Contract To Add

### Prepared Types

`src/backend/prealloc/prealloc.hpp` should grow a stronger control-flow model.
The current `PreparedBranchCondition` and `PreparedJoinTransfer` are the right
starting point, but they need more explicit semantics.

Suggested sketch:

```cpp
enum class PreparedBranchConditionKind {
  MaterializedBool,
  FusedCompare,
};

enum class PreparedJoinTransferKind {
  EdgeStoreSlot,
  ValueMaterialization,
  LoopCarry,
};

struct PreparedEdgeValueTransfer {
  std::string predecessor_label;
  std::string successor_label;
  bir::Value incoming_value;
  bir::Value destination_value;
  std::optional<std::string> storage_name;
};

struct PreparedBranchCondition {
  std::string function_name;
  std::string block_label;
  PreparedBranchConditionKind kind;
  bir::Value condition_value;
  std::optional<bir::BinaryOpcode> predicate;
  std::optional<bir::TypeKind> compare_type;
  std::optional<bir::Value> lhs;
  std::optional<bir::Value> rhs;
  bool can_fuse_with_branch = false;
  std::string true_label;
  std::string false_label;
};

struct PreparedJoinTransfer {
  std::string function_name;
  std::string join_block_label;
  PreparedJoinTransferKind kind;
  bir::Value destination_value;
  std::vector<PreparedEdgeValueTransfer> edge_transfers;
};
```

The exact names may change, but the contract must make these facts explicit:

- branch meaning
- join destination value
- edge-local incoming values
- loop backedge transfer membership

### Required Invariants

The prepared handoff must guarantee:

- each `CondBranch` block has at most one authoritative prepared condition
- each join destination value is described once in prepared control-flow data
- edge-to-join transfer data is keyed by predecessor and successor labels
- loop-carried values use the same transfer model as non-loop joins
- x86 may reject unsupported target operand forms, but it may not recover
  semantic meaning from CFG topology when prepared control-flow data exists

## Concrete Functions And Helpers

These functions should exist in shared code:

```cpp
PreparedBranchCondition build_prepared_branch_condition(const bir::Function& function,
                                                        const bir::Block& block);

std::vector<PreparedJoinTransfer> collect_prepared_join_transfers(
    const bir::Function& function);

void lower_phi_nodes_to_edge_transfers(bir::Function* function,
                                       std::vector<PreparedJoinTransfer>* transfers);

void materialize_select_joins(const bir::Function& function,
                              const std::vector<PreparedBranchCondition>& conditions,
                              std::vector<PreparedJoinTransfer>* transfers);

const PreparedBranchCondition* find_prepared_branch_condition(
    const PreparedControlFlowFunction& function_cf,
    std::string_view block_label);

std::span<const PreparedEdgeValueTransfer> incoming_transfers_for_join(
    const PreparedControlFlowFunction& function_cf,
    std::string_view join_block_label,
    const bir::Value& destination_value);
```

The production side belongs in:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`

The consumer side belongs in:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- later shared/x86 scalar lowering helpers from idea 59

## Producer / Consumer Boundary

### Produced By Shared Lowering

- branch-condition kind
- compare operands for fused compare-and-branch
- join destination value
- incoming edge transfers
- loop-carry classification

### Consumed By X86

- whether to emit `cmp/test + jcc` or branch on an already-materialized bool
- which incoming value reaches a join destination
- whether a loop backedge is ordinary join traffic rather than a special lane

### Must Not Be Re-Derived In X86

- short-circuit ownership from raw block chains
- join meaning from local-slot naming patterns
- loop-carried value identity from countdown-style matcher logic

## Code Example

### Source BIR Shape

```cpp
// entry:
//   %cmp = slt i32 %n, 10
//   condbr %cmp, then, else
// then:
//   br join
// else:
//   br join
// join:
//   %x = phi [1, then], [0, else]
//   ret %x
```

### Prepared Control-Flow Handoff

```cpp
PreparedBranchCondition{
  .function_name = "main",
  .block_label = "entry",
  .kind = PreparedBranchConditionKind::FusedCompare,
  .condition_value = bir::Value::named(TypeKind::I32, "%cmp"),
  .predicate = bir::BinaryOpcode::Slt,
  .compare_type = bir::TypeKind::I32,
  .lhs = bir::Value::named(TypeKind::I32, "%n"),
  .rhs = bir::Value::immediate_i32(10),
  .can_fuse_with_branch = true,
  .true_label = "then",
  .false_label = "else",
};

PreparedJoinTransfer{
  .function_name = "main",
  .join_block_label = "join",
  .kind = PreparedJoinTransferKind::ValueMaterialization,
  .destination_value = bir::Value::named(TypeKind::I32, "%x"),
  .edge_transfers = {
    {.predecessor_label = "then",
     .successor_label = "join",
     .incoming_value = bir::Value::immediate_i32(1),
     .destination_value = bir::Value::named(TypeKind::I32, "%x")},
    {.predecessor_label = "else",
     .successor_label = "join",
     .incoming_value = bir::Value::immediate_i32(0),
     .destination_value = bir::Value::named(TypeKind::I32, "%x")},
  },
};
```

With this contract, x86 no longer needs a special "materialized compare join"
lane. It just emits a branch from the prepared condition and emits the join
materialization from the prepared edge transfers.

## Acceptance Shape

This idea is satisfied when:

- prepared control-flow records are sufficient for ordinary branch and join
  emission without whole-shape recovery
- loop-carried transfers use the same contract family as non-loop joins
- the x86 consumer can be written as lookup-based control-flow lowering rather
  than `try_*` CFG matching

## Non-Goals

- assigning physical registers
- choosing concrete x86 arithmetic instructions
- stack-frame sizing
- final call ABI move planning
