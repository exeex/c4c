#include "src/backend/prealloc/formal_publications.hpp"
#include "src/backend/prealloc/prepared_lookups.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << "\n";
    return false;
  }
  return true;
}

bir::CallArgAbiInfo register_abi(bir::TypeKind type) {
  return bir::CallArgAbiInfo{
      .type = type,
      .size_bytes = 4,
      .align_bytes = 4,
      .primary_class = bir::AbiValueClass::Integer,
      .passed_in_register = true,
  };
}

bir::CallArgAbiInfo stack_abi(bir::TypeKind type) {
  return bir::CallArgAbiInfo{
      .type = type,
      .size_bytes = 4,
      .align_bytes = 4,
      .primary_class = bir::AbiValueClass::Integer,
      .passed_on_stack = true,
  };
}

struct Fixture {
  prepare::PreparedNameTables names;
  bir::Function function;
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedValueHomeLookups lookups;
};

Fixture make_fixture() {
  Fixture fixture;
  const auto function_name = fixture.names.function_names.intern("formal.plan");
  fixture.function.name = "formal.plan";
  fixture.locations.function_name = function_name;

  auto add_param = [&](std::string name,
                       bir::TypeKind type,
                       std::optional<bir::CallArgAbiInfo> abi,
                       bool is_varargs = false,
                       bool is_sret = false) {
    fixture.names.value_names.intern(name);
    fixture.function.params.push_back(bir::Param{
        .type = type,
        .name = std::move(name),
        .size_bytes = 4,
        .align_bytes = 4,
        .abi = std::move(abi),
        .is_varargs = is_varargs,
        .is_sret = is_sret,
    });
  };

  add_param("%reg", bir::TypeKind::I32, register_abi(bir::TypeKind::I32));
  add_param("%stack", bir::TypeKind::I32, stack_abi(bir::TypeKind::I32));
  add_param("%missing_home", bir::TypeKind::I32, register_abi(bir::TypeKind::I32));
  add_param("%missing_stack_offset", bir::TypeKind::I32, stack_abi(bir::TypeKind::I32));
  add_param("%varargs", bir::TypeKind::Ptr, std::nullopt, true, false);
  add_param("%sret", bir::TypeKind::Ptr, register_abi(bir::TypeKind::Ptr), false, true);
  add_param("%unsupported_home", bir::TypeKind::I32, register_abi(bir::TypeKind::I32));
  add_param("%missing_reg_name", bir::TypeKind::I32, register_abi(bir::TypeKind::I32));
  add_param("%missing_abi", bir::TypeKind::I32, std::nullopt);

  const auto reg_name = fixture.names.value_names.find("%reg");
  const auto stack_name = fixture.names.value_names.find("%stack");
  const auto missing_stack_name = fixture.names.value_names.find("%missing_stack_offset");
  const auto unsupported_name = fixture.names.value_names.find("%unsupported_home");
  const auto missing_reg_name = fixture.names.value_names.find("%missing_reg_name");
  const auto missing_abi_name = fixture.names.value_names.find("%missing_abi");

  fixture.locations.value_homes = {
      prepare::PreparedValueHome{
          .value_id = 1,
          .function_name = function_name,
          .value_name = reg_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = "r1",
      },
      prepare::PreparedValueHome{
          .value_id = 2,
          .function_name = function_name,
          .value_name = stack_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = 5,
          .offset_bytes = 40,
      },
      prepare::PreparedValueHome{
          .value_id = 4,
          .function_name = function_name,
          .value_name = missing_stack_name,
          .kind = prepare::PreparedValueHomeKind::StackSlot,
          .slot_id = 6,
      },
      prepare::PreparedValueHome{
          .value_id = 7,
          .function_name = function_name,
          .value_name = unsupported_name,
          .kind = prepare::PreparedValueHomeKind::RematerializableImmediate,
          .immediate_i32 = 3,
      },
      prepare::PreparedValueHome{
          .value_id = 8,
          .function_name = function_name,
          .value_name = missing_reg_name,
          .kind = prepare::PreparedValueHomeKind::Register,
      },
      prepare::PreparedValueHome{
          .value_id = 9,
          .function_name = function_name,
          .value_name = missing_abi_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = "r9",
      },
  };
  fixture.lookups = prepare::make_prepared_value_home_lookups(&fixture.locations);
  return fixture;
}

prepare::PreparedFormalPublicationInputs inputs_for(const Fixture& fixture) {
  return prepare::PreparedFormalPublicationInputs{
      .names = &fixture.names,
      .function = &fixture.function,
      .value_locations = &fixture.locations,
      .value_home_lookups = &fixture.lookups,
  };
}

int verify_available_publications() {
  const auto fixture = make_fixture();
  const auto inputs = inputs_for(fixture);
  const auto reg = prepare::plan_prepared_formal_publication(inputs, 0);
  const auto stack = prepare::plan_prepared_formal_publication(inputs, 1);

  if (!expect(prepare::prepared_formal_publication_available(reg),
              "register-home formal publication should be available") ||
      !expect(reg.action ==
                  prepare::PreparedFormalPublicationAction::IncomingRegisterToHome,
              "register-home formal should preserve incoming-register action") ||
      !expect(reg.formal_index == 0 && reg.formal == &fixture.function.params[0],
              "register-home formal metadata was not preserved") ||
      !expect(reg.value_id == 1 && reg.home == &fixture.locations.value_homes[0],
              "register-home prepared authority was not preserved") ||
      !expect(reg.home_kind == prepare::PreparedValueHomeKind::Register,
              "register-home kind mismatch")) {
    return 1;
  }

  if (!expect(prepare::prepared_formal_publication_available(stack),
              "stack-home formal publication should be available") ||
      !expect(stack.action ==
                  prepare::PreparedFormalPublicationAction::IncomingStackToHome,
              "stack-home formal should preserve incoming-stack action") ||
      !expect(stack.value_id == 2 && stack.home == &fixture.locations.value_homes[1],
              "stack-home prepared authority was not preserved") ||
      !expect(stack.home_kind == prepare::PreparedValueHomeKind::StackSlot,
              "stack-home kind mismatch")) {
    return 1;
  }
  return 0;
}

int verify_missing_and_unsupported_statuses() {
  const auto fixture = make_fixture();
  const auto inputs = inputs_for(fixture);
  const auto missing_home = prepare::plan_prepared_formal_publication(inputs, 2);
  const auto missing_stack = prepare::plan_prepared_formal_publication(inputs, 3);
  const auto unsupported = prepare::plan_prepared_formal_publication(inputs, 6);
  const auto missing_register = prepare::plan_prepared_formal_publication(inputs, 7);
  const auto missing_abi = prepare::plan_prepared_formal_publication(inputs, 8);

  if (!expect(missing_home.status ==
                  prepare::PreparedFormalPublicationStatus::MissingValueHome,
              "missing-home status mismatch") ||
      !expect(missing_stack.status ==
                  prepare::PreparedFormalPublicationStatus::MissingStackOffset,
              "missing stack-offset status mismatch") ||
      !expect(missing_stack.home == &fixture.locations.value_homes[2],
              "missing stack-offset should preserve home authority") ||
      !expect(unsupported.status ==
                  prepare::PreparedFormalPublicationStatus::UnsupportedHomeKind,
              "unsupported home-kind status mismatch") ||
      !expect(missing_register.status ==
                  prepare::PreparedFormalPublicationStatus::MissingRegisterName,
              "missing register-name status mismatch") ||
      !expect(missing_abi.status ==
                  prepare::PreparedFormalPublicationStatus::MissingAbiInfo,
              "missing ABI status mismatch")) {
    return 1;
  }
  return 0;
}

int verify_no_publication_and_collection() {
  const auto fixture = make_fixture();
  const auto inputs = inputs_for(fixture);
  const auto varargs = prepare::plan_prepared_formal_publication(inputs, 4);
  const auto sret = prepare::plan_prepared_formal_publication(inputs, 5);
  const auto plans = prepare::plan_prepared_formal_publications(inputs);

  if (!expect(varargs.status == prepare::PreparedFormalPublicationStatus::NoPublication,
              "varargs formal should report no publication") ||
      !expect(sret.status == prepare::PreparedFormalPublicationStatus::NoPublication,
              "sret formal should report no publication") ||
      !expect(plans.size() == fixture.function.params.size(),
              "formal publication collection size mismatch") ||
      !expect(plans[0].value_id == 1 &&
                  plans[4].status == prepare::PreparedFormalPublicationStatus::NoPublication,
              "formal publication collection did not preserve per-formal facts")) {
    return 1;
  }
  return 0;
}

}  // namespace

int main() {
  if (const auto status = verify_available_publications(); status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = verify_missing_and_unsupported_statuses(); status != 0) {
    return EXIT_FAILURE;
  }
  if (const auto status = verify_no_publication_and_collection(); status != 0) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
