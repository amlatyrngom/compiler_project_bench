#include <iostream>
#include "table.h"
#include "expansive.h"
#include <chrono>
#include <thread>


template <class Tp>
static inline void DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

template <class Tp>
static inline void DoNotOptimize(Tp const& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}


template <typename F>
double RunBench(uint32_t num_runs, F f) {
  double total_time{0};
  for (uint32_t i = 0; i < num_runs; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "Start Timing" << std::endl;
    f();
    std::cout << "End Timing" << std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    total_time += duration.count();
  }
  return (total_time / num_runs);
}

void FilterOrderingCostBench(uint32_t num_rows) {
  Table table({Distro::Seq, Distro::SmallDiv}, num_rows);

  auto cheap = [](int64_t col1, int64_t col2) {
    return col2 > 8;
  };

  auto expansive = [](int64_t col1, int64_t col2) {
    return col1 > 0 && col2 % col1 > 8;
  };



  auto ordered_time = RunBench(10, [&]() {
    TableIterator iter(&table);
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    while (iter.TableNext()) {
      auto col1 = iter.GetColumn(0);
      auto col2 = iter.GetColumn(1);
      if (cheap(col1, col2)) {
        if (expansive(col1, col2)) {
          count++;
          res.FillResult(col1);
          res.FillResult(col2);
        }
      }
    }
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "OPT COUNT : " << count << std::endl;
  });

  auto unordered_time = RunBench(10, [&]() {
    TableIterator iter(&table);
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    while (iter.TableNext()) {
      auto col1 = iter.GetColumn(0);
      auto col2 = iter.GetColumn(1);
      if (expansive(col1, col2)) {
        if (cheap(col1, col2)) {
          count++;
          res.FillResult(col1);
          res.FillResult(col2);
        }
      }
    }
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "UNOPT COUNT : " << count << std::endl;
  });
  std::cout << "Ordered Time: " << ordered_time << std::endl;
  std::cout << "Unordered Time: " << unordered_time << std::endl;
}

void FilterOrderingSelectivityBench(uint32_t num_rows) {
  Table table({Distro::Seq, Distro::SmallDiv}, num_rows);

  // 10 percent selectivity
  auto filter1 = [](int64_t col1, int64_t col2) {
    return col1 > 0 && col2 % col1 == 5;
  };

  // Around 90 percent selectivity
  auto filter2 = [](int64_t col1, int64_t col2) {
    return col2 > 0 && col1 % col2 < 6;
  };

  auto ordered_time = RunBench(10, [&]() {
    TableIterator iter(&table);
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    while (iter.TableNext()) {
      auto col1 = iter.GetColumn(0);
      auto col2 = iter.GetColumn(1);
      if (filter1(col1, col2)) {
        if (filter2(col1, col2)) {
          count++;
          res.FillResult(col1);
          res.FillResult(col2);
        }
      }
    }
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "OPT COUNT : " << count << std::endl;
  });

  auto unordered_time = RunBench(10, [&]() {
    TableIterator iter(&table);
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    while (iter.TableNext()) {
      auto col1 = iter.GetColumn(0);
      auto col2 = iter.GetColumn(1);
      if (filter2(col1, col2)) {
        if (filter1(col1, col2)) {
          count++;
          res.FillResult(col1);
          res.FillResult(col2);
        }
      }
    }
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "UNOPT COUNT : " << count << std::endl;
  });

  std::cout << "Ordered Time: " << ordered_time << std::endl;
  std::cout << "Unordered Time: " << unordered_time << std::endl;
}


void DDLICMBench(uint32_t num_rows, uint32_t fib_arg) {
  Table table({Distro::Seq, Distro::SmallDiv}, num_rows);
  int64_t unmoved_count = 0;
  Table unmoved_res({Distro::Seq, Distro::SmallDiv}, 0);

  auto filter = [](int64_t col1, int64_t col2) {
    return col2 > 5;
  };

  auto moved_time = RunBench(10, [&]() {
    auto fib_result = Fib(fib_arg);
    TableIterator iter(&table);
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    while (iter.TableNext()) {
      auto col1 = iter.GetColumn(0);
      auto col2 = iter.GetColumn(1);
      if (filter(col1, col2)) {
        count += fib_result;
        res.FillResult(col1);
        res.FillResult(col2);
      }
    }
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "OPT COUNT : " << count << std::endl;
  });

  auto unmoved_time = RunBench(10, [&]() {
    TableIterator iter(&table);
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    while (iter.TableNext()) {
      auto col1 = iter.GetColumn(0);
      auto col2 = iter.GetColumn(1);
      if (filter(col1, col2)) {
        count += Fib(fib_arg);
        res.FillResult(col1);
        res.FillResult(col2);
      }
    }
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "UNOPT COUNT : " << count << std::endl;
  });

  std::cout << "Moved Time: " << moved_time << std::endl;
  std::cout << "Unmoved Time: " << unmoved_time << std::endl;
}

void DDDCEBench(uint32_t num_rows) {
  Table table({Distro::Seq, Distro::SmallDiv}, num_rows);

  // Impossible filter
  auto filter = [](int64_t col1, int64_t col2) {
    return 1 > 2;
  };

  auto eliminated_time = RunBench(10, [&]() {
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "OPT COUNT : " << count << std::endl;
  });

  auto uneliminated_time = RunBench(10, [&]() {
    int64_t count = 0;
    Table res({Distro::Seq, Distro::SmallDiv}, 0);
    TableIterator iter(&table);
    // TODO: Make expansive by materializing tuple at each iteration.
    while (iter.TableNext()) {
      auto col1 = iter.GetColumn(0);
      auto col2 = iter.GetColumn(1);
      if (filter(col1, col2)) {
        count++;
        res.FillResult(col1);
        res.FillResult(col2);
      }
    }
    DoNotOptimize(res);
    DoNotOptimize(count);
    std::cout << "UNOPT COUNT : " << count << std::endl;
  });

  std::cout << "Eliminated Time: " << eliminated_time << std::endl;
  std::cout << "Uneliminated Time: " << uneliminated_time << std::endl;
}


void JoinOrderingBenchmark(uint32_t small_size, uint32_t medium_size, uint32_t large_size) {
  // Make Three Tables
  Table small({Distro::Seq, Distro::SmallDiv}, small_size);
  Table medium({Distro::Seq, Distro::SmallDiv}, medium_size);
  Table large({Distro::Seq, Distro::SmallDiv}, large_size);

  auto make_join = [&](Table* outer_table, Table* inner_table, Table* out_table) {
    TableIterator outer(outer_table);
    while (outer.TableNext()) {
      // Make intermediary table
      auto outer_col1 = outer.GetColumn(0);
      auto outer_col2 = outer.GetColumn(1);
      TableIterator inner(inner_table);
      while (inner.TableNext()) {
        auto inner_col1 = inner.GetColumn(0);
        auto inner_col2 = inner.GetColumn(1);
        if (outer_col1 == inner_col1) {
          out_table->FillResult(outer_col1);
          out_table->FillResult(outer_col2 + inner_col2);
        }
      }
    }
  };

  auto check_count = [&](Table* table) {
    TableIterator iter(table);
    int64_t count = 0;
    while (iter.TableNext()) {
      count += 1;
    }
    return count;
  };

  auto ordered_time = RunBench(10, [&]() {
    // Make Intermediate Table
    Table interm({Distro::Seq, Distro::SmallDiv}, 0);
    make_join(&medium, &small, &interm);
    // Make Final Table
    Table final({Distro::Seq, Distro::SmallDiv}, 0);
    make_join(&large, &interm, &final);
    // Check the count of the second row
    std::cout << "OPT COUNT : " << check_count(&final) << std::endl;
    DoNotOptimize(final);
  });

  auto unordered_time = RunBench(10, [&]() {
    // Make Intermediate Table
    Table interm({Distro::Seq, Distro::SmallDiv}, 0);
    make_join(&large, &medium, &interm);
    // Make Final Table
    Table final({Distro::Seq, Distro::SmallDiv}, 0);
    make_join(&small, &interm, &final);
    // Check the count of the second row
    std::cout << "UNOPT COUNT : " << check_count(&final) << std::endl;
    DoNotOptimize(final);
  });

  std::cout << "Ordered Time: " << ordered_time << std::endl;
  std::cout << "Unordered Time: " << unordered_time << std::endl;


}

int main() {
  FilterOrderingCostBench(10000000);
  FilterOrderingSelectivityBench(10000000);
  DDLICMBench(10000000, 10);
  DDDCEBench(100000000);
  JoinOrderingBenchmark(1000, 5000, 10000);

  return 0;
}