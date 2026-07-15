#include "src/backend/bir/core/ir.hpp"

#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace bir = c4c::backend::bir;

namespace {

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

}  // namespace

int main() {
  static_assert(std::is_same_v<bir::Opcode, bir::NodeKind>,
                "Opcode must remain only a compatibility name");
  static_assert(bir::is_semantic_node_kind_v<bir::NodeKind::Binary>);
  static_assert(bir::is_binary_node_kind_v<bir::NodeKind::Binary>);
  static_assert(!bir::is_binary_node_kind_v<bir::NodeKind::Select>);
  static_assert(
      bir::node_kind_descriptor<bir::NodeKind::Binary>().minimum_operands == 2);
  static_assert(
      bir::node_kind_descriptor<bir::NodeKind::Store>().result_arity ==
      bir::ResultArityPolicy::Zero);

  const auto load = bir::node_kind_descriptor(bir::NodeKind::Load);
  expect(load && load->family == bir::NodeFamily::Memory &&
             load->effects == bir::NodeEffect::ReadsMemory &&
             load->result_arity == bir::ResultArityPolicy::One,
         "load must publish memory, effect, and result policy");

  const auto call = bir::node_kind_descriptor(bir::NodeKind::Call);
  expect(call && call->family == bir::NodeFamily::Call &&
             call->operand_arity == bir::OperandArityPolicy::Variable,
         "call must publish its family and variable input policy");

  const auto authority =
      bir::node_kind_descriptor(bir::NodeKind::StackRestoreAuthority);
  expect(authority && authority->family == bir::NodeFamily::Authority &&
             authority->minimum_operands == 1 &&
             authority->result_arity == bir::ResultArityPolicy::Zero,
         "stack restore must publish authority arity policy");

  expect(bir::node_kind_accepts_payload(bir::NodeKind::Store,
                                        bir::InstPayload{bir::StoreNode{}}) &&
             bir::node_kind_accepts_payload(
                 bir::NodeKind::Store,
                 bir::InstPayload{bir::LocalStoreAuthorityNode{}}),
         "store must accept both durable payload alternatives");
  expect(bir::node_kind_accepts_payload(bir::NodeKind::Call,
                                        bir::InstPayload{bir::CallNode{}}) &&
             bir::node_kind_accepts_payload(
                 bir::NodeKind::Call,
                 bir::InstPayload{bir::IntrinsicCallNode{}}),
         "call must accept direct and intrinsic payload alternatives");
  expect(!bir::node_kind_accepts_payload(bir::NodeKind::Load,
                                         bir::InstPayload{bir::StoreNode{}}),
         "payload acceptance must reject a mismatched semantic kind");

  const auto invalid = static_cast<bir::NodeKind>(255);
  expect(!bir::node_kind_descriptor(invalid) &&
             !bir::is_semantic_node_kind(invalid) &&
             !bir::is_binary_node_kind(invalid) &&
             !bir::node_kind_legal_in(invalid, bir::NodeStage::Raw) &&
             !bir::node_kind_accepts_payload(
                 invalid, bir::InstPayload{bir::InlineAsmNode{}}),
         "invalid runtime kinds must fail closed for every query");
  expect(bir::node_kind_legal_in(bir::NodeKind::Binary,
                                 bir::NodeStage::Canonical),
         "schema must expose legal-stage policy");

  expect(bir::node_kind_accepts_arity(bir::NodeKind::Store, 1, 0) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Store, 0, 0) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Store, 1, 1),
         "fixed zero-result kinds must enforce both exact counts");
  expect(bir::node_kind_accepts_arity(bir::NodeKind::Binary, 2, 1) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Binary, 3, 1) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Binary, 2, 0),
         "fixed one-result kinds must enforce both exact counts");
  expect(bir::node_kind_accepts_arity(bir::NodeKind::Load, 0, 1) &&
             bir::node_kind_accepts_arity(bir::NodeKind::Load, 1, 1) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Load, 2, 1),
         "bounded variable inputs must preserve their compatibility range");
  expect(bir::node_kind_accepts_arity(bir::NodeKind::InlineAsm, 0, 0) &&
             bir::node_kind_accepts_arity(bir::NodeKind::InlineAsm, 5, 3),
         "many-result kinds must accept their declared open result policy");
  expect(!bir::node_kind_accepts_arity(invalid, 0, 0),
         "invalid kinds must fail closed in the generic arity helper");
}
