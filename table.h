#pragma once
#include <vector>
#include <deque>
#include <random>
#include <cstdint>

enum class Distro {
  Seq, Unif, Const, SmallDiv, Any
};

class Table {
 public:
  Table(const std::vector<Distro> & schema, uint32_t num_row) : table_{} {
    for (const auto col_type: schema) {
      switch (col_type) {
        case Distro::Seq: {
          table_.emplace_back(MakeSeqColumn(num_row));
          break;
        }
        case Distro::Unif: {
          table_.emplace_back(MakeUnifColumn(num_row));
          break;
        }
        case Distro::Const: {
          table_.emplace_back(MakeConstColumn(num_row));
          break;
        }
        case Distro::SmallDiv: {
          table_.emplace_back(MakeSmallDivColumn(num_row));
          break;
        }
        default: {
          break;
        }
      }
    }
  }

  std::deque<int64_t> MakeSeqColumn(uint32_t num_row) {
    std::deque<int64_t> ret;
    for (uint32_t i = 0; i < num_row; i++) {
      ret.emplace_back(i);
    }
    return std::move(ret);
  }

  std::deque<int64_t> MakeUnifColumn(uint32_t num_row) {
    int64_t min = -10000;
    int64_t max = 10000;
    std::default_random_engine generator(time(NULL));
    std::uniform_int_distribution<int64_t> distribution(min,max);

    std::deque<int64_t> ret;
    for (uint32_t i = 0; i < num_row; i++) {
      ret.emplace_back(distribution(generator));
    }
    return std::move(ret);
  }

  std::deque<int64_t> MakeConstColumn(uint32_t num_row) {
    std::deque<int64_t> ret;
    for (uint32_t i = 0; i < num_row; i++) {
      ret.emplace_back(0);
    }
    return std::move(ret);
  }

  std::deque<int64_t> MakeSmallDivColumn(uint32_t num_row) {
    int64_t min = 1;
    int64_t max = 10;
    std::default_random_engine generator(time(NULL));
    std::uniform_int_distribution<int64_t> distribution(min,max);

    std::deque<int64_t> ret;
    for (uint32_t i = 0; i < num_row; i++) {
      ret.emplace_back(distribution(generator));
    }
    return std::move(ret);
  }


  int64_t FetchValue(uint32_t row_idx, uint32_t col_idx) {
    return table_[col_idx][row_idx];
  }

  void FillResult(int64_t val) {
    curr_insert_row_.emplace_back(val);
    if (curr_insert_row_.size() == table_.size()) {
      for (uint32_t col_idx = 0; col_idx < curr_insert_row_.size(); col_idx++) {
        table_[col_idx].emplace_back(curr_insert_row_[col_idx]);
      }
      curr_insert_row_.clear();
    }
  }

  uint32_t NumRows() {
    return table_[0].size();
  }

 private:
  std::vector<std::deque<int64_t>> table_;
  std::vector<int64_t> curr_insert_row_{};
};


class TableIterator {
 public:
  TableIterator(Table* table) : table_(table) {}

  bool TableNext() {
    curr_row_++;
    return curr_row_ <= table_->NumRows();
  }

  int64_t GetColumn(uint32_t col_idx) {
    return table_->FetchValue(curr_row_ - 1, col_idx);
  }

 private:
  Table* table_;
  uint32_t curr_row_{0};
};