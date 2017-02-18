//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "csv_reader.h"
#include <testing/standalone_test.h>

TEST_CASE("csv reader reads small file", "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.open("csv/small.csv"));
  CHECK(reader.nextLine());
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "0");
  CHECK(reader.field(1) == "1");
  CHECK(reader.nextLine());
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "2");
  CHECK(reader.field(1) == "3");
  CHECK_FALSE(reader.nextLine());
}