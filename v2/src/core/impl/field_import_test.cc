//
// Created by Arseny Tolmachev on 2017/02/20.
//

#include "field_import.h"
#include "field_reader.h"

#include <testing/standalone_test.h>
#include <util/flatset.h>

using namespace jumanpp;
using namespace jumanpp::core::dic::impl;

bool feedData(util::CsvReader &csv, FieldImporter &importer) {
  while (csv.nextLine()) {
    bool status = importer.importFieldValue(csv);
    if (!status) return false;
  }
  return true;
}

TEST_CASE("string field importer processes storage") {
  StringPiece testData = "bug\nbeer\nblob\nbirch\nbug\ntoad";
  StringStorage ss;
  StringFieldImporter importer{&ss, 0, ""};
  util::CsvReader rdr;
  CHECK_OK(rdr.initFromMemory(testData));
  CHECK(feedData(rdr, importer));
  util::CodedBuffer buffer;
  CHECK_OK(importer.makeStorage(&buffer));
  StringStorageTraversal trav{buffer.contents()};
  util::FlatMap<StringPiece, i32> actualPositions;
  while (trav.hasNext()) {
    StringPiece sp;
    REQUIRE(trav.next(&sp));
    actualPositions[sp] = trav.position();
  }
  StringStorageReader storageReader{buffer.contents()};
  StringPiece piece;
  CHECK(actualPositions.size() == 6); // 5 + empty
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

void checkSLFld(IntStorageReader &intStorage, StringStorageReader &strings,
                i32 idx, std::vector<i32> &ptrs,
                std::initializer_list<StringPiece> values) {
  CAPTURE(idx);
  auto ptr = ptrs[idx];
  CAPTURE(ptr);

  auto stringPtrs = intStorage.listAt(ptr);
  i32 stringPtr = 0;
  util::FlatSet<StringPiece> set{std::begin(values), std::end(values)};
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
  util::CodedBuffer fieldData;
  StringStorage ss;
  StringListFieldImporter imp{&ss, 0, ""};
  imp.injectFieldBuffer(&fieldData);
  util::CsvReader csv;
  CHECK_OK(csv.initFromMemory(testdata));
  CHECK(feedData(csv, imp));
  util::CodedBuffer stringData;
  CHECK_OK(imp.makeStorage(&stringData));
  std::vector<i32> pointers;
  csv.initFromMemory(testdata);
  while (csv.nextLine()) {
    pointers.push_back(imp.fieldPointer(csv));
  }
  CHECK(pointers.size() == 6);
  IntStorageReader fieldCntReader{fieldData.contents()};
  StringStorageReader stringRdr{stringData.contents()};

  checkSLFld(fieldCntReader, stringRdr, 0, pointers, {"this", "is"});
  checkSLFld(fieldCntReader, stringRdr, 1, pointers, {"no", "more"});
  checkSLFld(fieldCntReader, stringRdr, 2, pointers, {});
  checkSLFld(fieldCntReader, stringRdr, 3, pointers, {"it", "is", "more"});
  checkSLFld(fieldCntReader, stringRdr, 4, pointers, {"no"});
}