#include "viterbi.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <omp.h>
namespace {

std::vector<int> get_ten_to_two(int x, int number_size){
    std::vector<int> two_number;
    for(int i=0; x>0; i++)
    {
        two_number.emplace_back(x%2);
        x= x/2;
    }
    for(int j=two_number.size(); j<number_size; j++){
        two_number.emplace_back(0);
    }
    return two_number;
}

template<typename T>
std::string bstring(T n, int num){
    std::string s;
    for(int m = num ;m--;){
            s.push_back('0'+((n >> m) & 1));
    }
    return s;
}

int HammingDistance(const std::string& x, const std::string& y) {
  assert(x.size() == y.size());
  int distance = 0;
  for (int i = 0; i < x.size(); i++) {
    distance += x[i] != y[i];
  }
  return distance;
}

}  // namespace

std::ostream& operator <<(std::ostream& os, const ViterbiCodec& codec) {
  os << "ViterbiCodec(" << codec.constraint() << ", {";
  const std::vector<int>& polynomials = codec.polynomials();
  assert(!polynomials.empty());
  os << polynomials.front();
  for (int i = 1; i < polynomials.size(); i++) {
    os << ", " << polynomials[i];
  }
  return os << "})";
}

int ReverseBits(int num_bits, int input) {
  assert(input < (1 << num_bits));
  int output = 0;
  while (num_bits-- > 0) {
    output = (output << 1) + (input & 1);
    input >>= 1;
  }
  return output;
}

ViterbiCodec::ViterbiCodec(int constraint, const std::vector<int>& polynomials)
    : constraint_(constraint), polynomials_(polynomials) {
  assert(!polynomials_.empty());
  for (int i = 0; i < polynomials_.size(); i++) {
    assert(polynomials_[i] > 0);
    assert(polynomials_[i] < (1 << constraint_));
  }
  InitializeOutputs();
}

int ViterbiCodec::num_parity_bits() const {
  return polynomials_.size();
}

int ViterbiCodec::NextState(int current_state, int input) const {
  return (current_state >> 1) | (input << (constraint_ - 2));
}

//std::string ViterbiCodec::Output(int current_state, int input) const {
//  return outputs_.at(current_state | (input << (constraint_ - 1)));
//}

std::string ViterbiCodec::Encode(const std::string& bits, int stride) const {
  std::string encoded;
//  int state = 0;
  std::string state(constraint_, '0');
  // Encode the message bits.
  for (int i = 0; i < bits.size(); i+=stride) {
    state.insert(state.begin(), bits.begin()+i, bits.begin()+i+stride);
    state.erase(state.begin()+constraint_,state.end());
//    char c = bits[i];
//    assert(c == '0' || c == '1');
//    int input = c - '0';
    std::string output;
    for(int i=0; i<polynomials_.size(); i++){
//        int l = input & reverse_outputs_[i];
        std::string n = bstring(reverse_outputs_[i], constraint_);
        int sum =0;
        for(int j=0; j<n.size(); j++){
            if(n[j]=='1' && state[j]=='1'){
                sum ++;
            }
        }
        if(sum%2==1){
            output +='1';
        }
        else{
            output +='0';
        }
    }
    encoded += output;
//    state = NextState(state, input);
  }
  return encoded;
}

void ViterbiCodec::InitializeOutputs() {
    reverse_outputs_.resize(num_parity_bits());
    for (int j = 0; j < num_parity_bits(); j++) {
      // Reverse polynomial bits to make the convolution code simpler.
      reverse_outputs_[j] =  ReverseBits(constraint_, polynomials_[j]);
    }
//  outputs_.resize(1 << constraint_);
//  for (int i = 0; i < outputs_.size(); i++) {
//    for (int j = 0; j < num_parity_bits(); j++) {
//      // Reverse polynomial bits to make the convolution code simpler.
//      int polynomial = ReverseBits(constraint_, polynomials_[j]);
//      int input = i;
//      int output = 0;
//      for (int k = 0; k < constraint_; k++) {
//        output ^= (input & 1) & (polynomial & 1);
//        polynomial >>= 1;
//        input >>= 1;
//      }
//      outputs_[i] += output ? "1" : "0";
//    }
//  }
}

int ViterbiCodec::BranchMetric(const std::string& bits,
                               int source_state,
                               int target_state, int stride) const {
  assert(bits.size() == num_parity_bits());
  int this_index = ((target_state>>(constraint_ - stride - stride))<<(constraint_ - stride)) | source_state;
  std::string output;
  for(int i=0; i<polynomials_.size(); i++){
      int l = this_index & reverse_outputs_[i];
      std::string n = bstring(l, constraint_);
      int sum =0;
      for(int j=0; j<n.size(); j++){
          if(n[j]=='1'){
              sum ++;
          }
      }
      if(sum%2==1){
          output +='1';
      }
      else{
          output +='0';
      }
  }
  return HammingDistance(bits, output);
}

std::vector<medium_vary_num> ViterbiCodec::PathMetric(
    const std::string& bits,
    int this_score, int source_index,
    int state, int stride, int deadline_top) const {
    std::vector<medium_vary_num> save_score_state;
    for(int i=0; i<deadline_top; i++){
        int s = (i<<(constraint_- stride - stride)) | (state>>stride);
        medium_vary_num score;
        score.source_state = source_index;
        score.target_state = s;
        score.score = BranchMetric(bits, state, s, stride) + this_score;
        save_score_state.emplace_back(score);
    }
    return save_score_state;
}

void ViterbiCodec::UpdatePathMetrics(const std::string& bits,
                                     std::vector<std::pair<int, int>>& path_metrics,
                                     Trellis& trellis,
                                      int stride, int deadline_top, std::vector<int>& this_limit) const {
  std::vector<medium_vary_num> temp_save_result;
  for (int i = 0; i < this_limit.size(); i++) {
          std::vector<medium_vary_num> now_score_state = PathMetric(bits, path_metrics[i].second, i, this_limit[i], stride, deadline_top);
          temp_save_result.insert(temp_save_result.begin(), now_score_state.begin(), now_score_state.end());
        }

  std::sort(temp_save_result.begin(), temp_save_result.end(), [](medium_vary_num& a, medium_vary_num& b){return a.score<b.score;});
  int count_num=1;
  std::vector<int> temp_save_index{temp_save_result[0].target_state};
  std::vector<int> source_index{temp_save_result[0].source_state};
  path_metrics[0].first = temp_save_result[0].target_state;
  path_metrics[0].second = temp_save_result[0].score;

  for(int i=1; i<temp_save_result.size(); i++){
      auto this_index = std::find(temp_save_index.begin(), temp_save_index.end(), temp_save_result[i].target_state);
      if(this_index != temp_save_index.end()){
          continue;
      }
      else{
          path_metrics[i].first = temp_save_result[i].target_state;
          path_metrics[i].second = temp_save_result[i].score;
          temp_save_index.emplace_back(temp_save_result[i].target_state);
          source_index.emplace_back(temp_save_result[i].source_state);
          count_num ++;
          if(count_num==10){
              break;
          }
      }
  }
  this_limit = temp_save_index;

  if(temp_save_index.size()<10){
      temp_save_index.insert(temp_save_index.end(), 10 - temp_save_index.size(), 0);
  }

  if(trellis.empty()){
        trellis.emplace_back(temp_save_index);
  }
  else{
      std::vector<std::vector<int>> temp_soure_route = trellis;
      int this_index = std::min(10, static_cast<int>(source_index.size()));
      for(int i=0; i<this_index; i++){
          for(int j=0; j<temp_soure_route.size(); j++)
          trellis[j][i] =  temp_soure_route[j][source_index[i]];
      }
      trellis.emplace_back(temp_save_index);
  }
}




std::string ViterbiCodec::Decode(const std::string& bits, int stride) const {
  // Compute path metrics and generate trellis.
  Trellis trellis;
  std::vector<std::pair<int, int>> path_metrics;
  for(int j=0; j<10; j++){
      path_metrics.emplace_back(std::pair<int,int>{j, std::numeric_limits<int>::max()});
  }
  path_metrics[0].second =0;
  int deadline_top = pow(2, stride);
  std::vector<int> this_limit{0};
  for (int i = 0; i < bits.size(); i += num_parity_bits()) {
    std::string current_bits(bits, i, num_parity_bits());
    // If some bits are missing, fill with trailing zeros.
    // This is not ideal but it is the best we can do.
    if (current_bits.size() < num_parity_bits()) {
      current_bits.append(
          std::string(num_parity_bits() - current_bits.size(), '0'));
    }
    UpdatePathMetrics(current_bits, path_metrics, trellis, stride, deadline_top, this_limit);
  }

  // Traceback.
  std::string decoded,temp_decode;
  int state = std::min_element(path_metrics.begin(), path_metrics.end()) - path_metrics.begin();
  for (int i = trellis.size() - 1; i >= 0; i--) {
      std::string this_trans = bstring(trellis[i][0], constraint_- stride);
      for(int j=0; j<stride; j++){
          temp_decode +=this_trans[j];
      }
  }
  for(int i=temp_decode.size(); i > 0; i-=stride){
      int begin_index = i-stride;
      int end_index = i;
      for(int j= begin_index; j<end_index; j++){
          decoded +=temp_decode[j];
      }
  }

  return decoded;
}

