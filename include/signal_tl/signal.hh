#pragma once

#if !defined(__SIGNAL_TEMPORAL_LOGIC_SIGNAL_HH__)
#define __SIGNAL_TEMPORAL_LOGIC_SIGNAL_HH__

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace signal_tl {
namespace signal {

struct Sample {
  double time;
  double value;
  double derivative = 0.0;

  /**
   * Linear interpolate the Sample (given the derivative) to get the value at time `t`.
   */
  constexpr double interpolate(double t) const {
    return value + derivative * (t - time);
  }

  /**
   * Get the time point at which the lines associated with this Sample and the given
   * Sample intersect.
   */
  constexpr double time_intersect(const Sample& point) const {
    return (value - point.value + (point.derivative * point.time) -
            (derivative * time)) /
           (point.derivative - derivative);
  }

  constexpr double area(double t) const {
    if (t > time) {
      return (value + this->interpolate(t)) * (t - time) / 2;
    } else {
      return 0;
    }
  }

  friend std::ostream& operator<<(std::ostream& out, const Sample& sample);

}; // namespace signal

constexpr bool operator<(const Sample& lhs, const Sample& rhs) {
  return lhs.value < rhs.value;
}
constexpr bool operator>(const Sample& lhs, const Sample& rhs) {
  return rhs < lhs;
}
constexpr bool operator<=(const Sample& lhs, const Sample& rhs) {
  return !(lhs > rhs);
}
constexpr bool operator>=(const Sample& lhs, const Sample& rhs) {
  return !(lhs < rhs);
}

constexpr Sample operator-(const Sample& other) {
  return {other.time, -other.value, -other.derivative};
}

/**
 * Piecewise-linear, right-continuous signal
 */
struct Signal {
 private:
  std::vector<Sample> samples;

 public:
  double begin_time() const {
    return (samples.empty()) ? 0.0 : samples.front().time;
  }

  double end_time() const {
    return (samples.empty()) ? 0.0 : samples.back().time;
  }

  double interpolate(double t, size_t idx) const {
    return this->samples.at(idx).interpolate(t);
  }

  double time_intersect(const Sample& point, size_t idx) const {
    return this->samples.at(idx).time_intersect(point);
  }

  double area(double t, size_t idx) const {
    return this->samples.at(idx).area(t);
  }

  Sample front() const {
    return this->samples.front();
  }

  Sample back() const {
    return this->samples.back();
  }

  Sample at_idx(size_t i) const {
    return this->samples.at(i);
  }

  /**
   * Get the sample at time `t`.
   *
   * Does a binary search for the given time instance, and interpolates from
   * the closest sample less than `t` if necessary.
   */
  Sample at(double t) const;
  /**
   * Get const_iterator to the start of the signal
   */
  auto begin() const {
    return this->samples.cbegin();
  }

  /**
   * Get const_iterator to the end of the signal
   */
  auto end() const {
    return this->samples.cend();
  }

  /**
   * Get const_iterator to the first element of the signal that is timed at or after
   * `s`
   */
  auto begin_at(double s) const {
    if (this->begin_time() >= s)
      return this->begin();

    constexpr auto comp_op = [](const Sample& a, const Sample& b) {
      return a.time < b.time;
    };
    return std::lower_bound(this->begin(), this->end(), Sample{s, 0.0}, comp_op);
  }

  /**
   * Get const_iterator to the element after the last element of the signal
   * that is timed at or before `t`
   */
  auto end_at(double t) const {
    if (this->end_time() <= t)
      return this->end();

    auto it = this->end();
    while (it->time > t) it = std::prev(it);
    // Now we have the pointer to the first element from the back whose .time <= t.
    // So increment by 1 and return
    return std::next(it);
  }

  /**
   * Get const reverse_iterator to the samples.
   */
  auto rbegin() const {
    return this->samples.crbegin();
  }

  /**
   * Get const reverse_iterator to the samples.
   */
  auto rend() const {
    return this->samples.crend();
  }

  size_t size() const {
    return this->samples.size();
  }

  bool empty() const {
    return this->samples.empty();
  }

  /**
   * Add a Sample to the back of the Signal
   */
  void push_back(Sample s);
  void push_back(double time, double value);

  /**
   * Remove sampling points where (y, dy) is continuous
   */
  std::shared_ptr<Signal> simplify() const;
  /**
   * Restrict/extend the signal to [s,t] with default value v where not defined.
   */
  std::shared_ptr<Signal> resize(double start, double end, double fill) const;
  /**
   * Shift the signal by dt time units
   */
  std::shared_ptr<Signal> shift(double dt) const;

  /**
   * Resize and shift a signal without creating copies. We use this often, so it makes
   * sense to combine it.
   */
  std::shared_ptr<Signal>
  resize_shift(double start, double end, double fill, double dt) const;

  Signal() : samples{} {}

  Signal(const Signal& other) {
    this->samples.reserve(other.size());
    for (const auto s : other) { this->push_back(s); }
  }

  /**
   * Create a Signal from a sequence of amples
   */
  template <
      typename T,
      typename = decltype(std::begin(std::declval<T>())),
      typename = decltype(std::end(std::declval<T>()))>
  Signal(const T& data) {
    this->samples.reserve(data.size());
    for (const auto s : data) { this->push_back(s); }
  }

  /**
   * Create a Signal from a sequence of data points and time stamps
   */
  Signal(const std::vector<double>& points, const std::vector<double>& times) {
    if (points.size() != times.size()) {
      throw std::invalid_argument(
          "Number of sample points and time points need to be equal.");
    }

    size_t n = points.size();
    this->samples.reserve(n);
    for (size_t i = 0; i < n; i++) { this->push_back(times.at(i), points.at(i)); }
  }

  /**
   * Create a Signal from the given iterators
   */
  template <
      typename T,
      typename TIter = decltype(std::begin(std::declval<T>())),
      typename       = decltype(std::end(std::declval<T>()))>
  Signal(TIter&& start, TIter&& end) {
    for (auto i = start; i != end; i++) { this->push_back(*i); }
  }

  friend std::ostream& operator<<(std::ostream& os, const Signal& sig);
};

/**
 * Synchronize two signals by making sure that one is explicitely defined for all the
 * time instances the other is defined.
 *
 * The output signals are confined to the time range where both of them are defined,
 * thus can truncate a signal if the other isn't defined there.
 */
std::tuple<std::shared_ptr<Signal>, std::shared_ptr<Signal>>
synchronize(const std::shared_ptr<Signal>& x, const std::shared_ptr<Signal>& y);

using SignalPtr = std::shared_ptr<Signal>;
using Trace     = std::map<std::string, SignalPtr>;

} // namespace signal
} // namespace signal_tl

#endif
