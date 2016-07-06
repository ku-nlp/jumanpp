#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN // main関数を定義
#include <boost/test/unit_test.hpp>
 
BOOST_AUTO_TEST_SUITE(test_jumanpp)
     
BOOST_AUTO_TEST_CASE(sample1)
{
    BOOST_CHECK_EQUAL(2*2, 4);
}
 
BOOST_AUTO_TEST_CASE(sample2)
{
    BOOST_CHECK_EQUAL(2*3, 6);
}

BOOST_AUTO_TEST_SUITE_END()
