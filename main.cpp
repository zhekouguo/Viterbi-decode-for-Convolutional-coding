#include<iostream>
#include "viterbi.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
//#include <omp.h>
void TestViterbiDecoding(const ViterbiCodec& codec,
                         const std::string& received,
                         const std::string& message,
                         int stride) {
  const std::string decoded = codec.Decode(received, stride);
  std::cout << std::string(60, '=') << std::endl
//            << codec << std::endl
            << "received = " << received << std::endl
            << "decoded  = " << decoded << std::endl;

  assert(decoded == message);

  std::cout << std::endl;
}

template<typename T>
std::string bstring(T n, int num){
    std::string s;
    for(int m = num ;m--;){
            s.push_back('0'+((n >> m) & 1));
    }
    return s;
}




int main()
{
    std::vector<int> polynomials;
    //-----------50------------//
    polynomials.push_back(3361);
    polynomials.push_back(1618);
    polynomials.push_back(636);
    polynomials.push_back(2239);
    ViterbiCodec codec(12, polynomials);
    //----------encode---------//
//    std::string bits = "000101001101010101110011000000001001011110110011111011111111110000000000";
//    std::cout << codec.Encode(bits,3) << std::endl;
    //----------decode--------//
    TestViterbiDecoding(codec,
                    "000010101011111110101010101000000100001101101000010100011000000011101010100110101001100001001010",
                    "000101001101010101110011000000001001011110110011111011111111110000000000", 3);
    //-------------------------//
    //----------80--------//
//    polynomials.push_back(9622725);
//    polynomials.push_back(2041222);
//    polynomials.push_back(8967723);
//    ViterbiCodec codec(24, polynomials);
//    TestViterbiDecoding(codec,
//                        "000011111101101110010001101000100011000010000101000110000110010011101010101110110010011111010010101110110101010110101000000000000000000000000000000000",
//                        "000111001010000000111101000000111111100010111010101110110000000000000000000",2);

    //---------100--------------//
//    polynomials.push_back(34789);
//    polynomials.push_back(59483);
//    ViterbiCodec codec(16, polynomials);
//    TestViterbiDecoding(codec,
//                        "000000111000101001011011010111110100001101011001011100001000001011001000110100111101111110101011001011010010101111001110101010110000111000101100000000",
//                        "000111001010000000111101000000111111100010111010101110110000000000000000000",1);
    //--------------------------//

    //------------140------------//
//    polynomials.push_back(13457);
//    polynomials.push_back(12185);
//    polynomials.push_back(15031);
//    polynomials.push_back(16055);
    //---------------------------//
    return 0;
}
