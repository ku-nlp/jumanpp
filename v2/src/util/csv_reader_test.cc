//
// Created by Arseny Tolmachev on 2017/02/18.
//

#include "csv_reader.h"
#include <testing/standalone_test.h>

TEST_CASE("csv reader reads small file", "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.open("csv/small.csv"));
  CHECK(reader.lineNumber() == 0);
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "0");
  CHECK(reader.field(1) == "1");
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 2);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "2");
  CHECK(reader.field(1) == "3");
  CHECK_FALSE(reader.nextLine());
}

TEST_CASE("csv reader reads small file with crlf", "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("0,1\r\n2,3\r\n"));
  CHECK(reader.lineNumber() == 0);
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "0");
  CHECK(reader.field(1) == "1");
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 2);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "2");
  CHECK(reader.field(1) == "3");
  CHECK_FALSE(reader.nextLine());
}

TEST_CASE("csv reader works with quoted fields", "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("0,\"1,2\"\n\"2\n5\",3\n6,\"9\""));
  CHECK(reader.lineNumber() == 0);
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "0");
  CHECK(reader.field(1) == "1,2");
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 2);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "2\n5");
  CHECK(reader.field(1) == "3");
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 3);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "6");
  CHECK(reader.field(1) == "9");
  CHECK_FALSE(reader.nextLine());
}

TEST_CASE("csv reader works with quoted and escaped fields", "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("\"\"\"\",\"1,2\"\"\"\n\"2,5\",3"));
  CHECK(reader.lineNumber() == 0);
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "\"");
  CHECK(reader.field(1) == "1,2\"");
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 2);
  CHECK(reader.numFields() == 2);
  CHECK(reader.field(0) == "2,5");
  CHECK(reader.field(1) == "3");
  CHECK_FALSE(reader.nextLine());
}

TEST_CASE("csv reader rejects invalid field escape in beginning of line",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("\"\"\",1"));
  CHECK_FALSE(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
}

TEST_CASE("csv reader rejects invalid field escape in middle of line",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("1,\"\"\",1"));
  CHECK_FALSE(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
}

TEST_CASE("csv reader rejects invalid field escape in end of line",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("1,\"\"\""));
  CHECK_FALSE(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
}

TEST_CASE("csv reader rejects invalid field escape in end of line2",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("1,\"\"\"\n"));
  CHECK_FALSE(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
}

TEST_CASE("csv reader rejects non-closed literal", "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("1,\"\n"));
  CHECK_FALSE(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
}

TEST_CASE("csv reader accepts two quotes a row in first field",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("\"\"\"\"\"\",1\n"));
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.field(0) == "\"\"");
}

TEST_CASE("csv reader accepts two quotes a row in first field with prefix",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("\"a\"\"\"\"\",1\n"));
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.field(0) == "a\"\"");
}

TEST_CASE("csv reader accepts two quotes a row in first field with suffix",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("\"\"\"\"\"a\",1\n"));
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.field(0) == "\"\"a");
}

TEST_CASE("csv reader accepts two quotes a row in second field",
          "[csv_reader]") {
  jumanpp::util::CsvReader reader;
  CHECK_OK(reader.initFromMemory("1,\"\"\"\"\"\"\n"));
  CHECK(reader.nextLine());
  CHECK(reader.lineNumber() == 1);
  CHECK(reader.field(0) == "1");
  CHECK(reader.field(1) == "\"\"");
}