#include "bir_to_backend_ir.hpp"

#include <stdexcept>

namespace c4c::backend {

namespace {

BackendScalarType lower_scalar_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I32:
      return BackendScalarType::I32;
    case bir::TypeKind::Void:
    case bir::TypeKind::I64:
      break;
  }
  throw std::invalid_argument("bir scaffold backend lowering only supports i32 scalar values");
}

std::string lower_type(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::Void:
      return "void";
    case bir::TypeKind::I32:
      return "i32";
    case bir::TypeKind::I64:
      return "i64";
  }
  throw std::invalid_argument("unsupported bir type");
}

std::string lower_value(const bir::Value& value) {
  if (value.kind == bir::Value::Kind::Named) {
    return value.name;
  }
  return std::to_string(value.immediate);
}

BackendInst lower_inst(const bir::BinaryInst& inst) {
  BackendBinaryInst lowered;
  lowered.opcode = BackendBinaryOpcode::Add;
  lowered.result = inst.result.name;
  lowered.type_str = lower_type(inst.result.type);
  lowered.lhs = lower_value(inst.lhs);
  lowered.rhs = lower_value(inst.rhs);
  lowered.value_type = lower_scalar_type(inst.result.type);
  return lowered;
}

BackendTerminator lower_terminator(const bir::ReturnTerminator& terminator) {
  if (!terminator.value.has_value()) {
    return make_backend_return(std::nullopt, "void");
  }
  return make_backend_return(lower_value(*terminator.value),
                             lower_type(terminator.value->type));
}

}  // namespace

BackendModule lower_to_backend_ir(const bir::Module& module) {
  BackendModule lowered;
  lowered.target_triple = module.target_triple;
  lowered.data_layout = module.data_layout;

  for (const auto& bir_function : module.functions) {
    BackendFunction function;
    function.is_declaration = bir_function.is_declaration;
    function.signature.linkage_kind = bir_function.is_declaration
                                          ? BackendFunctionLinkage::Declare
                                          : BackendFunctionLinkage::Define;
    function.signature.return_type = lower_type(bir_function.return_type);
    function.signature.return_type_kind =
        parse_backend_value_type_kind(function.signature.return_type);
    function.signature.return_scalar_type =
        parse_backend_value_scalar_type(function.signature.return_type);
    function.signature.name = bir_function.name;

    for (const auto& bir_block : bir_function.blocks) {
      BackendBlock block;
      block.label = bir_block.label;
      for (const auto& inst : bir_block.insts) {
        block.insts.push_back(lower_inst(inst));
      }
      block.terminator = lower_terminator(bir_block.terminator);
      function.blocks.push_back(std::move(block));
    }

    lowered.functions.push_back(std::move(function));
  }

  return lowered;
}

}  // namespace c4c::backend
