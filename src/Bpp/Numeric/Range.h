//
// File: Range.h
// Created by: Julien Dutheil
// Created on: Mon Nov 21 15:52 2011
//

/*
   Copyright or © or Copr. Bio++ Development Team, (November 17, 2004)

   This software is a computer program whose purpose is to provide classes
   for numerical calculus.

   This software is governed by the CeCILL  license under French law and
   abiding by the rules of distribution of free software.  You can  use,
   modify and/ or redistribute the software under the terms of the CeCILL
   license as circulated by CEA, CNRS and INRIA at the following URL
   "http://www.cecill.info".

   As a counterpart to the access to the source code and  rights to copy,
   modify and redistribute granted by the license, users are provided only
   with a limited warranty  and the software's author,  the holder of the
   economic rights,  and the successive licensors  have only  limited
   liability.

   In this respect, the user's attention is drawn to the risks associated
   with loading,  using,  modifying and/or developing or reproducing the
   software by the user in light of its specific status of free software,
   that may mean  that it is complicated to manipulate,  and  that  also
   therefore means  that it is reserved for developers  and  experienced
   professionals having in-depth computer knowledge. Users are therefore
   encouraged to load and test the software's suitability as regards their
   requirements in conditions enabling the security of their systems and/or
   data to be ensured and,  more generally, to use and operate it in the
   same conditions as regards security.

   The fact that you are presently reading this means that you have had
   knowledge of the CeCILL license and that you accept its terms.
 */

#ifndef _RANGE_H_
#define _RANGE_H_

#include "../Text/TextTools.h"

//From the STL:
#include <string>
#include <set>
#include <algorithm>

namespace bpp {

/**
 * @brief The Range class, defining an interval.
 *
 * Methods are provided for extending the range, get union and intersection.
 */
template<class T> class Range
{
  private:
    T begin_;
    T end_;

  public:
    /**
     * @brief Creates a new interval.
     *
     * If a > b, then the positions are swapped.
     * If a == b, the interval is considered empty.
     *
     * @param a First position
     * @param b Second position
     */
    Range(const T& a = 0, const T& b = 0):
      begin_(std::min(a, b)),
      end_(std::max(a, b))
    {}

  public:
    bool operator==(const Range<T>& r) const {
      return begin_ == r.begin_ && end_ == r.end_;
    }
    bool operator!=(const Range<T>& r) const {
      return begin_ != r.begin_ || end_ != r.end_;
    }
    bool operator<(const Range<T>& r) const {
      return begin_ < r.begin_ || end_ < r.end_;
    }

    T begin() const { return begin_; }
    
    T end() const { return end_; }

    /**
     * @param r Range to compare with.
     * @return True if the two intervals overlap.
     */
    bool overlap(const Range& r) const
    {
      if (r.begin_ <= begin_ && r.end_ >= end_)
        return true;
      else if (r.begin_ >= begin_ && r.begin_ <= end_)
        return true;
      else if (r.end_ >= begin_ && r.end_ <= end_)
        return true;
      else
        return false;
    }

    /**
     * @brief Expand the current interval with the given one.
     *
     * If the two intervals do not overlap, then the interval is not modified.
     * @param r input interval.
     */
    void expandWith(const Range& r)
    {
      if (r.begin_ < begin_ && r.end_ >= begin_)
        begin_ = r.begin_;
      if (r.end_ > end_ && r.begin_ <= end_)
        end_ = r.end_;
    }

    /**
     * @brief Restrict the current interval to the intersection with the given one.
     *
     * If the two intervals do not overlap, then the interval is set to empty.
     * @param r input interval.
     */
    void sliceWith(const Range& r)
    {
      if (!overlap(r)) {
        begin_ = 0;
        end_   = 0;
      } else {
        if (r.begin_ > begin_ && r.begin_ <= end_)
          begin_ = r.begin_;
        if (r.end_ < end_ && r.end_ >= begin_)
          end_ = r.end_;
      }
    }

    /**
     * @return True if then begining position equals the ending one.
     */
    bool isEmpty() const { return begin_ == end_; }

    /**
     * @return A string describing the range.
     */
    std::string toString() const {
      return ("[" + TextTools::toString(begin_) + "," + TextTools::toString(end_) + "[");
    }

};

/**
 * @brief Interface discribing a collection of Range objects.
 */
template<class T> class RangeCollection {
  public:
    virtual ~RangeCollection() {}
    /**
     * @brief Add a new range to the collection.
     *
     * @param r The range to add to the collection.
     */
    virtual void addRange(const Range<T>& r) = 0;

    /**
     * @brief Get the intersection with a given range.
     *
     * The new multirange is the union of all ranges intersections with the given range.
     *
     * @param r Restriction range.
     */
    virtual void restrictTo(const Range<T>& r) = 0;

    /**
     * @return A string representation of the set of intervals.
     */
    virtual std::string toString() const = 0;

    /**
     * @return True if the set does not contain any range.
     */
    virtual bool isEmpty() const = 0;
};

/**
 * @brief This class implements a data structure describing a set of intervales.
 *
 * Intervales can be overlapping.
 */
template<class T> class RangeSet:
  public RangeCollection<T>
{
  public:

  private:
    std::set< Range<T> > ranges_;

  public:
    RangeSet(): ranges_() {}

  public:
    void addRange(const Range<T>& r) {
      ranges_.insert(r);
    }

    void restrictTo(const Range<T>& r) {
      std::set < Range<T> > bck = ranges_;
      ranges_.clear();
      for (typename std::set< Range<T> >::iterator it = bck.begin(); it != bck.end(); ++it) {
        Range<T> rc = *it;
        rc.sliceWith(r);
        ranges_.insert(rc);
      }
    }

    std::string toString() const {
      std::string s = "{ ";
      for (typename std::set< Range<T> >::const_iterator it = ranges_.begin(); it != ranges_.end(); ++it) {
        s += it->toString() + " ";
      }
      s += "}";
      return s;
    }

    bool isEmpty() const { return ranges_.size() == 0; }

};


/**
 * @brief This class implements a data structure describing a set of non-overlapping intervales.
 */
template<class T> class MultiRange:
  public RangeCollection<T>
{
  private:
    std::vector< Range<T> > ranges_;

  public:
    MultiRange(): ranges_() {}

  public:
    void addRange(const Range<T>& r) {
      //this is a bit tricky, as many cases can happen. we have to check how many ranges overlap with the new one:
      std::vector<size_t> overlappingPositions;
      for (size_t i = 0; i < ranges_.size(); ++i) {
        if (ranges_[i].overlap(r))
          overlappingPositions.push_back(i);
      }
      //check if not overlap:
      if (overlappingPositions.size() == 0) {
        //We simply add the new range to the list:
        ranges_.push_back(r);
      } else {
        //We extand the first overlapping element:
        ranges_[overlappingPositions[0]].expandWith(r);
        //Now we merge all other overlapping ranges, if any:
        for (size_t i = overlappingPositions.size() - 1; i > 0; --i) {
          //Expand first range:
          ranges_[overlappingPositions[0]].expandWith(ranges_[overlappingPositions[i]]);
          //Then removes this range:
          ranges_.erase(ranges_.begin() + overlappingPositions[i]);
        }
      }
      clean_();
    }

    /**
     * @brief Get the intersection with a given range.
     *
     * The new multirange is the union of all ranges intersections with the given range.
     *
     * @param r Restriction range.
     */
    void restrictTo(const Range<T>& r) {
      for (typename std::vector< Range<T> >::iterator it = ranges_.begin(); it != ranges_.end(); ++it) {
        it->sliceWith(r);
      }
      clean_();
    }

    /**
     * @return A string representation of the set of intervals.
     */
    std::string toString() const {
      std::string s = "{ ";
      for (typename std::vector< Range<T> >::const_iterator it = ranges_.begin(); it != ranges_.end(); ++it) {
        s += it->toString() + " ";
      }
      s += "}";
      return s;
    }

    /**
     * @return A vector with all interval bounds.
     */
    std::vector<T> getBounds() const {
      std::vector<T> bounds;
      for (typename std::vector< Range<T> >::const_iterator it = ranges_.begin(); it != ranges_.end(); ++it) {
        bounds.push_back(it->begin());
        bounds.push_back(it->end());
      }
      return bounds;
    }

    /**
     * @return True if the set does not contain any range.
     */
    bool isEmpty() const { return ranges_.size() == 0; }



  private:
    void clean_() {
      //Reorder
      std::sort(ranges_.begin(), ranges_.end());
      //Remove empty intervals:
      for (size_t i = ranges_.size(); i > 0; --i) {
        if (ranges_[i - 1].isEmpty())
          ranges_.erase(ranges_.begin() + i - 1);
      }
    }
};

} //end of namespace bpp

#endif //_RANGE_H_
