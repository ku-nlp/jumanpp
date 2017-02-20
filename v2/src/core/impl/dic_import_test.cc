//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "dic_import.h"
#include <testing/standalone_test.h>

using namespace jumanpp;
using namespace jumanpp::core::dic::impl;

bool feedData(util::CsvReader& csv, FieldImporter& importer) {
  while (csv.nextLine()) {
    bool status = importer.importFieldValue(csv);
    if (!status) return false;
  }
  return true;
}

TEST_CASE("string field importer processes storage") {
  StringPiece testData = "bug\nbeer\nblob\nbirch\nbug\ntoad";
  StringFieldImporter importer{0};
  util::CsvReader rdr;
  CHECK_OK(rdr.initFromMemory(testData));
  CHECK(feedData(rdr, importer));
  std::stringstream buffer;
  CHECK_OK(importer.makeStorage(buffer));
  auto data = buffer.str();
  StringDomainTraversal trav{data};
  util::FlatMap<StringPiece, i32> actualPositions;
  while (trav.hasNext()) {
    StringPiece sp;
    REQUIRE(trav.next(&sp));
    actualPositions[sp] = trav.position();
  }
  CHECK(actualPositions.size() == 5);
  CHECK_OK(rdr.initFromMemory(testData));
  CHECK(rdr.nextLine());
  CHECK(importer.pointer(rdr) == actualPositions["bug"]);
  CHECK(rdr.nextLine());
  CHECK(importer.pointer(rdr) == actualPositions["beer"]);
  CHECK(rdr.nextLine());
  CHECK(importer.pointer(rdr) == actualPositions["blob"]);
  CHECK(rdr.nextLine());
  CHECK(importer.pointer(rdr) == actualPositions["birch"]);
  CHECK(rdr.nextLine());
  CHECK(importer.pointer(rdr) == actualPositions["bug"]);
  CHECK(rdr.nextLine());
  CHECK(importer.pointer(rdr) == actualPositions["toad"]);
  CHECK_FALSE(rdr.nextLine());
}