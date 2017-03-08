//
// Created by Arseny Tolmachev on 2017/03/09.
//

#include "testing/standalone_test.h"
#include "mikolov_rnn.h"
#include "util/mmap.h"

using namespace jumanpp;
using namespace jumanpp::rnn::mikolov;

TEST_CASE("can read mikonov rnn header") {
  util::MappedFile file;
  REQUIRE_OK(file.open("rnn/testlm.nnet", util::MMapType::ReadOnly));
  util::MappedFileFragment frag;
  REQUIRE_OK(file.map(&frag, 0, file.size()));
  MikolovRnnModelHeader header;
  REQUIRE_OK(readHeader(frag.asStringPiece(), &header));
  CHECK(header.layerSize == 20);
  CHECK(header.nceLnz == 9);
  CHECK(header.maxentOrder == 3);
}