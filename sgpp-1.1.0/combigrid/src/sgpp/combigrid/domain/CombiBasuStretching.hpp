// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef COMBIBASISTRETCHING_HPP_
#define COMBIBASISTRETCHING_HPP_
#include <sgpp/combigrid/domain/AbstractStretchingMaker.hpp>
#include <sgpp/globaldef.hpp>

#include <vector>

/**
 *   Implement a point  stretching
 *
 *   Nirmal Kumar Basu; Madhav Chandra Kundu
 *
 *
 *
 */

namespace combigrid {

class CombiBasuStretching : public AbstractStretchingMaker {
 public:
  /// do nada ...
  CombiBasuStretching() { ; }

  virtual ~CombiBasuStretching() { ; }

  /**
   * @param level - integer specifying the current grid level . the
   * corresponding nr of points is 2^level + 1
   * @param min - the left boundary of the interval
   * @param max - the right boundary of the interval
   * @param stretching - the output vector of pre-computed grid points...
   * @param jacobian - the evaluated jacobian at all points of the stretching ,
   * taking into consideration
   * size of the interval and underlying tranformations.
   */

  void get1DStretching(int level, double min, double max, std::vector<double>* stretching,
                       std::vector<double>* jacobian) const;

  Stretching getStretchingType() const { return BASU; }
};
}  // namespace combigrid

#endif /* COMBIBASISTRETCHING_HPP_ */
