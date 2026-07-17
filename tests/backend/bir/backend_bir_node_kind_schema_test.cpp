#include "src/backend/bir/core/ir.hpp"

#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace bir = c4c::backend::bir;

namespace {

constexpr bir::NodeKindSchema fixture_schema(
    bir::NodeSemanticFamily family, bir::NodeStage owner,
    bir::NodeMirDisposition mir,
    bir::NodeSsaParticipation ssa = bir::NodeSsaParticipation::NeverSsa) {
  return {bir::NodeKind::InlineAsm,
          {bir::NodeFamily::Semantic, false,
           bir::OperandArityPolicy::Fixed, 0, 0,
           bir::ResultArityPolicy::Zero, bir::NodeEffect::None,
           static_cast<std::uint8_t>(owner)},
          bir::NodeValueModel::NoOrdinaryResult,
          ssa,
          family,
          0,
          bir::NodeControlBehavior::FallsThrough,
          bir::NodeTrapBehavior::CannotTrap,
          owner,
          bir::NodeTypePolicy::NoResultType,
          bir::NodePayloadPolicy::OneClosedPayload,
          mir};
}

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
  static_assert(bir::node_kind_count == 18);
  static_assert(bir::detail::validate_node_kind_registry());
  static_assert(bir::is_semantic_node_kind_v<bir::NodeKind::Binary>);
  static_assert(bir::is_binary_node_kind_v<bir::NodeKind::Binary>);
  static_assert(!bir::is_binary_node_kind_v<bir::NodeKind::Unary>);
  static_assert(!bir::is_binary_node_kind_v<bir::NodeKind::Select>);
  static_assert(
      bir::node_kind_descriptor<bir::NodeKind::Binary>().minimum_operands == 2);
  static_assert(
      bir::node_kind_descriptor<bir::NodeKind::Unary>().minimum_operands == 1);
  static_assert(
      bir::node_kind_descriptor<bir::NodeKind::Store>().result_arity ==
      bir::ResultArityPolicy::Zero);
  static_assert(
      bir::node_has_tag_v<bir::NodeKind::Binary, bir::NodeTag::SsaEligible>);
  static_assert(
      bir::is_ssa_eligible_in_v<bir::NodeKind::Binary,
                                bir::NodeStage::Canonical>);
  static_assert(bir::is_value_producing_v<bir::NodeKind::Phi>);
  static_assert(bir::is_memory_op_v<bir::NodeKind::Store>);
  static_assert(bir::may_write_memory_v<bir::NodeKind::Store>);
  static_assert(!bir::is_terminator_v<bir::NodeKind::Store>);
  static_assert(!bir::node_kind_admitted_in_v<
                bir::NodeKind::Binary, bir::NodeStage::PseudoPreallocation>);

  constexpr auto binary_authoring_spec = bir::detail::NodeKindSpec{
      .kind = bir::NodeKind::Binary,
      .legacy_family = bir::NodeFamily::Semantic,
      .value_model = bir::NodeValueModel::SingleOrdinaryResult,
      .ssa = bir::NodeSsaParticipation::SsaEligible,
      .semantic_family = bir::NodeSemanticFamily::Arithmetic,
      .refinements =
          bir::detail::refinements<bir::NodeKindRefinement::BinaryForm>(),
      .operands = bir::detail::fixed_arity(2),
      .results = bir::ResultArityPolicy::One,
      .effects = bir::NodeEffect::None,
      .control = bir::NodeControlBehavior::FallsThrough,
      .trap = bir::NodeTrapBehavior::MayTrap,
      .stage_owner = bir::NodeStage::Raw,
      .admitted_stages = bir::detail::semantic_stages,
      .type_policy = bir::NodeTypePolicy::StoredValueType,
      .mir = bir::NodeMirDisposition::OneRecordRealizable,
  };
  constexpr auto invalid_binary_authoring_spec = [=] {
    auto spec = binary_authoring_spec;
    spec.operands = bir::detail::fixed_arity(1);
    return spec;
  }();
  constexpr auto invalid_stage_owner_spec = [=] {
    auto spec = binary_authoring_spec;
    spec.stage_owner = static_cast<bir::NodeStage>(
        static_cast<std::uint8_t>(bir::NodeStage::Raw) |
        static_cast<std::uint8_t>(bir::NodeStage::Canonical));
    return spec;
  }();
  static_assert(bir::detail::valid_authoring_spec(binary_authoring_spec));
  static_assert(
      !bir::detail::valid_authoring_spec(invalid_binary_authoring_spec));
  static_assert(!bir::detail::valid_authoring_spec(invalid_stage_owner_spec));
  static_assert(bir::detail::variable_arity(1).maximum ==
                bir::detail::unbounded_arity);
  static_assert((bir::detail::semantic_stages.bits &
                 static_cast<std::uint8_t>(bir::NodeStage::Prepared)) != 0);

  constexpr auto prepared = fixture_schema(
      bir::NodeSemanticFamily::Preparation, bir::NodeStage::Prepared,
      bir::NodeMirDisposition::RequiresExpansion);
  constexpr auto pseudo = fixture_schema(
      bir::NodeSemanticFamily::Pseudo, bir::NodeStage::PseudoPreallocation,
      bir::NodeMirDisposition::RequiresAllocationOrFrameFacts);
  constexpr auto machine = fixture_schema(
      bir::NodeSemanticFamily::Machine, bir::NodeStage::MirReadyMachine,
      bir::NodeMirDisposition::MachineOnly);
  constexpr auto invalid_ssa = fixture_schema(
      bir::NodeSemanticFamily::Arithmetic, bir::NodeStage::Canonical,
      bir::NodeMirDisposition::OneRecordRealizable,
      bir::NodeSsaParticipation::SsaEligible);
  static_assert(bir::detail::validate_node_kind_schema(prepared));
  static_assert(bir::detail::validate_node_kind_schema(pseudo));
  static_assert(bir::detail::validate_node_kind_schema(machine));
  static_assert(!bir::detail::validate_node_kind_schema(invalid_ssa));

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

  const auto va_start =
      bir::node_kind_descriptor(bir::NodeKind::VaStartAuthority);
  expect(va_start && va_start->family == bir::NodeFamily::Authority &&
             va_start->effects == bir::NodeEffect::WritesMemory &&
             va_start->minimum_operands == 1 &&
             va_start->result_arity == bir::ResultArityPolicy::Zero,
         "va_start must be a registered Raw authority memory write");
  expect(bir::node_kind_accepts_payload(
             bir::NodeKind::VaStartAuthority,
             bir::InstPayload{bir::VaStartAuthorityNode{}}) &&
             bir::node_kind_legal_in(bir::NodeKind::VaStartAuthority,
                                     bir::NodeStage::Raw),
         "va_start payload and Raw admission must come from the registry");

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
  const auto invalid_tag = static_cast<bir::NodeTag>(255);
  const auto invalid_stage = static_cast<bir::NodeStage>(64);
  expect(!bir::node_kind_descriptor(invalid) &&
             !bir::node_kind_schema(invalid) &&
             !bir::is_semantic_node_kind(invalid) &&
             !bir::is_binary_node_kind(invalid) &&
             !bir::node_kind_legal_in(invalid, bir::NodeStage::Raw) &&
             !bir::node_kind_legal_in(bir::NodeKind::Binary, invalid_stage) &&
             !bir::node_has_tag(invalid, bir::NodeTag::SsaEligible) &&
             !bir::node_has_tag(bir::NodeKind::Binary, invalid_tag) &&
             !bir::is_ssa_eligible_in(invalid, bir::NodeStage::Canonical) &&
             !bir::node_kind_accepts_payload(
                 invalid, bir::InstPayload{bir::InlineAsmNode{}}),
         "invalid runtime kinds must fail closed for every query");
  expect(bir::node_kind_legal_in(bir::NodeKind::Binary,
                                 bir::NodeStage::Canonical),
         "schema must expose legal-stage policy");
  expect(bir::node_kind_legal_in(bir::NodeKind::Binary,
                                 bir::NodeStage::Prepared) &&
             !bir::node_kind_legal_in(
                 bir::NodeKind::Binary,
                 bir::NodeStage::PseudoPreallocation) &&
             !bir::node_kind_legal_in(bir::NodeKind::Binary,
                                      bir::NodeStage::Allocated) &&
             !bir::node_kind_legal_in(
                 bir::NodeKind::Binary,
                 bir::NodeStage::MirReadyMachine),
         "semantic kinds must admit immutable preparation references but reject later vocabularies");
  expect(bir::node_has_tag(bir::NodeKind::Binary,
                           bir::NodeTag::SsaEligible) ==
                 bir::node_has_tag_v<bir::NodeKind::Binary,
                                     bir::NodeTag::SsaEligible> &&
             bir::is_ssa_eligible_in(bir::NodeKind::Binary,
                                     bir::NodeStage::Canonical) ==
                 bir::is_ssa_eligible_in_v<bir::NodeKind::Binary,
                                           bir::NodeStage::Canonical> &&
             bir::is_value_producing(bir::NodeKind::Phi) ==
                 bir::is_value_producing_v<bir::NodeKind::Phi> &&
             bir::may_write_memory(bir::NodeKind::Store) ==
                 bir::may_write_memory_v<bir::NodeKind::Store>,
         "compile-time and runtime queries must agree from one registry");

  expect(bir::node_kind_accepts_arity(bir::NodeKind::Store, 1, 0) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Store, 0, 0) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Store, 1, 1),
         "fixed zero-result kinds must enforce both exact counts");
  expect(bir::node_kind_accepts_arity(bir::NodeKind::Binary, 2, 1) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Binary, 3, 1) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Binary, 2, 0),
         "fixed one-result kinds must enforce both exact counts");
  expect(bir::node_kind_accepts_arity(bir::NodeKind::Unary, 1, 1) &&
             !bir::node_kind_accepts_arity(bir::NodeKind::Unary, 2, 1) &&
             bir::node_kind_accepts_payload(
                 bir::NodeKind::Unary,
                 bir::InstPayload{bir::BinaryNode{}}),
         "unary arithmetic must have its own fixed-arity registry entry");
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
