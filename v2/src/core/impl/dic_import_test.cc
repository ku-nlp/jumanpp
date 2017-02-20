//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "dic_import.h"
#include <testing/standalone_test.h>
#include <util/flatset.h>

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
  StringStorageTraversal trav{data};
  util::FlatMap<StringPiece, i32> actualPositions;
  while (trav.hasNext()) {
    StringPiece sp;
    REQUIRE(trav.next(&sp));
    actualPositions[sp] = trav.position();
  }
  StringStorageReader storageReader{data};
  ;
  StringPiece piece;
  CHECK(actualPositions.size() == 5);
  CHECK_OK(rdr.initFromMemory(testData));
  CHECK(rdr.nextLine());
  auto ptr = importer.fieldPointer(rdr);
  CHECK(ptr == actualPositions["bug"]);
  CHECK(storageReader.readAt(ptr, &piece));
  CHECK(piece == "bug");
  CHECK(rdr.nextLine());
  auto bugPtr = ptr;
  ptr = importer.fieldPointer(rdr);
  CHECK(ptr == actualPositions["beer"]);
  CHECK(storageReader.readAt(ptr, &piece));
  CHECK(piece == "beer");
  CHECK(rdr.nextLine());
  ptr = importer.fieldPointer(rdr);
  CHECK(ptr == actualPositions["blob"]);
  CHECK(storageReader.readAt(ptr, &piece));
  CHECK(piece == "blob");
  CHECK(rdr.nextLine());
  ptr = importer.fieldPointer(rdr);
  CHECK(ptr == actualPositions["birch"]);
  CHECK(storageReader.readAt(ptr, &piece));
  CHECK(piece == "birch");
  CHECK(rdr.nextLine());
  ptr = importer.fieldPointer(rdr);
  CHECK(ptr == actualPositions["bug"]);
  CHECK(storageReader.readAt(ptr, &piece));
  CHECK(piece == "bug");
  CHECK(ptr == bugPtr);
  CHECK(rdr.nextLine());
  ptr = importer.fieldPointer(rdr);
  CHECK(ptr == actualPositions["toad"]);
  CHECK(storageReader.readAt(ptr, &piece));
  CHECK(piece == "toad");
  CHECK_FALSE(rdr.nextLine());
}

void checkOne(IntStorageReader& intStorage, StringStorageReader& strings,
              i32 idx, std::vector<i32>& ptrs,
              std::initializer_list<StringPiece> values) {
  CAPTURE(idx);
  auto ptr = ptrs[idx];
  CAPTURE(ptr);

  auto stringPtrs = intStorage.listAt(ptr);
  i32 stringPtr = 0;
  util::FlatSet<StringPiece> set{values};
  CHECK(set.size() == stringPtrs.size());
  CHECK(set.size() == stringPtrs.remaining());

  for (i32 i = 0; i < stringPtrs.size(); ++i) {
    CAPTURE(i);
    StringPiece sp;
    CHECK(stringPtrs.readOneCumulative(&stringPtr));
    CHECK(strings.readAt(stringPtr, &sp));
    CHECK(set.count(sp) == 1);
  }

  CHECK(stringPtrs.remaining() == 0);
}

TEST_CASE("string list field processes input") {
  StringPiece testdata{"this is\nno more\n\nis it more\nno\n\n"};
  std::stringstream fieldStream;
  StringListFieldImporter imp{0, fieldStream};
  util::CsvReader csv;
  CHECK_OK(csv.initFromMemory(testdata));
  CHECK(feedData(csv, imp));
  std::stringstream stringData;
  CHECK_OK(imp.makeStorage(stringData));
  std::vector<i32> pointers;
  csv.initFromMemory(testdata);
  while (csv.nextLine()) {
    pointers.push_back(imp.fieldPointer(csv));
  }
  CHECK(pointers.size() == 6);
  auto fieldContentData = fieldStream.str();
  IntStorageReader fieldCntReader{fieldContentData};
  auto stringDataRaw = stringData.str();
  StringStorageReader stringRdr{stringDataRaw};

  checkOne(fieldCntReader, stringRdr, 0, pointers, {"this", "is"});
  checkOne(fieldCntReader, stringRdr, 1, pointers, {"no", "more"});
  checkOne(fieldCntReader, stringRdr, 2, pointers, {});
  checkOne(fieldCntReader, stringRdr, 3, pointers, {"it", "is", "more"});
  checkOne(fieldCntReader, stringRdr, 4, pointers, {"no"});
}