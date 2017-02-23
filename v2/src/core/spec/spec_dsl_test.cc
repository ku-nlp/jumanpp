//
// Created by Arseny Tolmachev on 2017/02/23.
//

#include "core/spec/spec_dsl.h"
#include "testing/standalone_test.h"

using namespace jumanpp::core::spec::dsl;

#define VALID(x) CHECK_OK((x).validate());
#define NOT_VALID(x) CHECK(!(x).validate().isOk());

TEST_CASE("create and validate fields") {
  ModelSpecBuilder spec;
  VALID(spec.field(1, "test").strings());
  VALID(spec.field(2, "asdf").stringLists());
  VALID(spec.field(3, "when").integers());
  VALID(spec.field(4, "index").strings().trieIndex());
  VALID(spec.field(5, "index").strings().emptyValue("asd"));
  VALID(spec.field(6, "index").stringLists().emptyValue("asd"));
  VALID(spec.field(7, "index").strings().emptyValue("asd").trieIndex());
  NOT_VALID(spec.field(8, "z").stringLists().trieIndex());
  NOT_VALID(spec.field(1, "z").integers().trieIndex());
  NOT_VALID(spec.field(1, "z").integers().emptyValue("nil"));
  NOT_VALID(spec.field(1, "").integers().emptyValue("nil"));
  NOT_VALID(spec.field(-1, "x").strings().emptyValue("nil"));
  NOT_VALID(spec.field(0, "x").strings().emptyValue("nil"));
}

TEST_CASE("fully valid field spec") {
  ModelSpecBuilder spec;
  VALID(spec.field(1, "test").strings());
  VALID(spec.field(2, "asdf").stringLists());
  VALID(spec.field(3, "when").integers());
  VALID(spec.field(4, "index").strings().trieIndex());
  CHECK_OK(spec.validateFields());
}

TEST_CASE("check failed with non-unique field numbers") {
  ModelSpecBuilder spec;
  VALID(spec.field(1, "test").strings());
  VALID(spec.field(1, "asdf").stringLists());
  CHECK_FALSE(spec.validateFields());
}

TEST_CASE("check failed with non-unique field names") {
  ModelSpecBuilder spec;
  VALID(spec.field(1, "test").strings());
  VALID(spec.field(2, "test").stringLists());
  CHECK_FALSE(spec.validateFields());
}

TEST_CASE("feature length transform") {
  ModelSpecBuilder spec;
  auto& f1 = spec.field(1, "f1").strings().trieIndex();
  auto& f1l = spec.feature("f1l").length(f1);
  VALID(f1l);
  VALID(spec);
}

TEST_CASE("feature value match") {
  ModelSpecBuilder spec;
  auto& f1 = spec.field(1, "f1").strings().trieIndex();
  auto& f2 = spec.field(2, "f2").strings();
  auto& f3 = spec.field(3, "f3").strings();
  auto& ft1 = spec.feature("ft1")
                  .matchValue(f1, "test")
                  .ifTrue({f2.value(), f3.value()})
                  .ifFalse({f3.value()});
  VALID(ft1);
  VALID(spec);
}