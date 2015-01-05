
#include <boost/test/unit_test.hpp>
#include "wvector.h"

BOOST_AUTO_TEST_SUITE(test_wvector)

BOOST_AUTO_TEST_CASE(wv_str)
{
    WeightedVector wvec;
    wvec.vec.push_back(std::make_pair(1,1.01) );
    wvec.vec.push_back(std::make_pair(3,1.3) );
    wvec.vec.push_back(std::make_pair(5,1.5) );
    wvec.vec.push_back(std::make_pair(7,1.7) );
    wvec.vec.push_back(std::make_pair(9,1.9) );
    wvec.vec.push_back(std::make_pair(10,1.10) );

    BOOST_CHECK_EQUAL(wvec.str(), "1:1.01 3:1.3 5:1.5 7:1.7 9:1.9 10:1.1 ");
}

BOOST_AUTO_TEST_CASE(wv_plus)
{
    WeightedVector wvec1,wvec2;
    wvec1.vec.push_back(std::make_pair(1,1.01) );
    wvec1.vec.push_back(std::make_pair(3,1.3) );
    wvec1.vec.push_back(std::make_pair(5,1.5) );
    wvec1.vec.push_back(std::make_pair(7,1.7) );
    wvec1.vec.push_back(std::make_pair(9,1.9) );
    wvec1.vec.push_back(std::make_pair(10,1.10) );
    
    wvec2.vec.push_back(std::make_pair(1,1.01) );
    wvec2.vec.push_back(std::make_pair(3,1.3) );
    wvec2.vec.push_back(std::make_pair(8,1.5) );
    wvec2.vec.push_back(std::make_pair(9,1.7) );
    wvec2.vec.push_back(std::make_pair(10,1.9) );
    wvec2.vec.push_back(std::make_pair(11,1.10) );
    wvec2.vec.push_back(std::make_pair(12,1.10) );

    WeightedVector* result = wvec1 + wvec2;
    BOOST_CHECK_EQUAL(result->str(), "1:2.02 3:2.6 5:1.5 7:1.7 8:1.5 9:3.6 10:3 11:1.1 12:1.1 ");
    delete(result);
}

BOOST_AUTO_TEST_CASE(wv_product)
{
    WeightedVector wvec1,wvec2;
    wvec1.vec.push_back(std::make_pair(1,1.01) );
    wvec1.vec.push_back(std::make_pair(3,1.3) );
    wvec1.vec.push_back(std::make_pair(5,1.5) );
    wvec1.vec.push_back(std::make_pair(7,1.7) );
    wvec1.vec.push_back(std::make_pair(9,1.9) );
    wvec1.vec.push_back(std::make_pair(10,1.10) );
    
    wvec2.vec.push_back(std::make_pair(1,1.01) );
    wvec2.vec.push_back(std::make_pair(3,1.3) );
    wvec2.vec.push_back(std::make_pair(8,1.5) );
    wvec2.vec.push_back(std::make_pair(9,1.7) );
    wvec2.vec.push_back(std::make_pair(10,1.9) );
    wvec2.vec.push_back(std::make_pair(11,1.10) );
    wvec2.vec.push_back(std::make_pair(12,1.10) );

    double result = wvec1.product(wvec2);
    BOOST_CHECK_EQUAL(result, 1.01*1.01 + 1.3*1.3 + 1.7*1.9 + 1.1*1.9);
    
    double result2 = wvec1.product(wvec1);
    BOOST_CHECK_EQUAL(result2, 1.01*1.01 + 1.3*1.3 + 1.5*1.5 + 1.7*1.7 + 1.9*1.9 + 1.1*1.1);
}

BOOST_AUTO_TEST_CASE(wv_merge)
{
    WeightedVector wvec1, wvec2;
    wvec1.vec.push_back(std::make_pair(1,1.01) );
    wvec1.vec.push_back(std::make_pair(3,1.3) );
    wvec1.vec.push_back(std::make_pair(5,1.5) );
    wvec1.vec.push_back(std::make_pair(7,1.7) );
    wvec1.vec.push_back(std::make_pair(9,1.9) );
    wvec1.vec.push_back(std::make_pair(10,1.10) );
    
    wvec2.vec.push_back(std::make_pair(1,1.01) );
    wvec2.vec.push_back(std::make_pair(3,1.3) );
    wvec2.vec.push_back(std::make_pair(8,1.5) );
    wvec2.vec.push_back(std::make_pair(9,1.7) );
    wvec2.vec.push_back(std::make_pair(10,1.9) );
    wvec2.vec.push_back(std::make_pair(11,1.10) );
    wvec2.vec.push_back(std::make_pair(12,1.10) );

    wvec1.merge(1.0, wvec2);
    BOOST_CHECK_EQUAL(wvec1.str(), "1:2.02 3:2.6 5:1.5 7:1.7 8:1.5 9:3.6 10:3 11:1.1 12:1.1 ");
    
    wvec1.merge(2.0, wvec1);
    BOOST_CHECK_EQUAL(wvec1.str(), "1:6.06 3:7.8 5:4.5 7:5.1 8:4.5 9:10.8 10:9 11:3.3 12:3.3 ");
}

BOOST_AUTO_TEST_CASE(wv_canonical)
{
    WeightedVector wvec1;
    BOOST_CHECK_EQUAL(wvec1.canonical, false);

    wvec1.vec.push_back(std::make_pair(3,1.3) );
    wvec1.vec.push_back(std::make_pair(5,1.5) );
    wvec1.vec.push_back(std::make_pair(10,1.10) );
    wvec1.vec.push_back(std::make_pair(1,1.01) );
    wvec1.vec.push_back(std::make_pair(9,1.9) );
    wvec1.vec.push_back(std::make_pair(10,1.10) );
    wvec1.vec.push_back(std::make_pair(7,1.7) );
    
    wvec1.canonicalize();
    BOOST_CHECK_EQUAL(wvec1.canonical, true);
    BOOST_CHECK_EQUAL(wvec1.str(), "1:1.01 3:1.3 5:1.5 7:1.7 9:1.9 10:2.2 ");
}

BOOST_AUTO_TEST_CASE(wv_add1)
{
    WeightedVector wvec1;
    BOOST_CHECK_EQUAL(wvec1.canonical, false);
    wvec1.canonicalize();
    BOOST_CHECK_EQUAL(wvec1.canonical, true);
    wvec1.add(3,1.3);
    wvec1.add(5,1.5);
    wvec1.add(10,1.1);
    wvec1.add(1,1.01);
    wvec1.add(9,1.9);
    wvec1.add(10,1.10);
    wvec1.add(7,1.7);
    BOOST_CHECK_EQUAL(wvec1.canonical, false);
    
    BOOST_CHECK_EQUAL(wvec1.str(), "1:1.01 3:1.3 5:1.5 7:1.7 9:1.9 10:2.2 ");
}

BOOST_AUTO_TEST_CASE(wv_add2)
{
    WeightedVector wvec1, wvec2;
    BOOST_CHECK_EQUAL(wvec1.canonical, false);
    wvec1.canonicalize();
    BOOST_CHECK_EQUAL(wvec1.canonical, true);
    wvec1.add("3",1.3); //0で始まる(mac:clang3.5svn)1で始まる(linux:gcc4.9)?
    wvec1.add("5",1.5);
    wvec1.add("10",1.1);
    wvec1.add("1",1.01);
    wvec1.add("9",1.9);
    wvec1.add("10",1.10);
    wvec1.add("7",1.7);
    BOOST_CHECK_EQUAL(wvec1.canonical, false);

    wvec2.add("a",1.3);
    wvec2.add("b",1.5);
    wvec2.add("c",1.1);
    wvec2.add("a",1.01);
    wvec2.add("b",1.9);
    wvec2.add("c",1.10);
    wvec2.add("d",1.7);
    
    BOOST_CHECK_EQUAL(wvec1.merge(-1.0, wvec2).str(), "0:1.3 1:1.5 2:2.2 3:1.01 4:1.9 5:1.7 6:-2.31 7:-3.4 8:-2.2 9:-1.7 ");
    BOOST_CHECK_EQUAL(wvec2.canonical, true);
    BOOST_CHECK_EQUAL(wvec2.canonical, true);
}

BOOST_AUTO_TEST_SUITE_END()
