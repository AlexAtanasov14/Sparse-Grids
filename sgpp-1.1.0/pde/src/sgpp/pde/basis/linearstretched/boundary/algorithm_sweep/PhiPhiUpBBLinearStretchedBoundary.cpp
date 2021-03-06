// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#include <sgpp/pde/basis/linearstretched/boundary/algorithm_sweep/PhiPhiUpBBLinearStretchedBoundary.hpp>

#include <sgpp/globaldef.hpp>

namespace SGPP {
namespace pde {

PhiPhiUpBBLinearStretchedBoundary::PhiPhiUpBBLinearStretchedBoundary(
    SGPP::base::GridStorage* storage)
    : PhiPhiUpBBLinearStretched(storage) {}

PhiPhiUpBBLinearStretchedBoundary::~PhiPhiUpBBLinearStretchedBoundary() {}

void PhiPhiUpBBLinearStretchedBoundary::operator()(SGPP::base::DataVector& source,
                                                   SGPP::base::DataVector& result,
                                                   grid_iterator& index, size_t dim) {
  float_t q = this->stretching->getIntervalWidth(dim);
  //  float_t t = this->stretching->getIntervalOffset(dim);

  // get boundary values
  float_t fl = 0.0;
  float_t fr = 0.0;

  if (!index.hint()) {
    index.resetToLevelOne(dim);

    if (!this->storage->end(index.seq())) {
      // This will be changed to rec
      rec(source, result, index, dim, fl, fr);
    }

    index.resetToLeftLevelZero(dim);
  }

  size_t seq_left;
  size_t seq_right;

  // left boundary
  seq_left = index.seq();

  // right boundary
  index.resetToRightLevelZero(dim);
  seq_right = index.seq();

  // up
  //////////////////////////////////////
  // Left
  if (this->stretching->hasDirichletBoundaryLeft(dim)) {
    result[seq_left] = 0.0;  // source[seq_left];
  } else {
    result[seq_left] = fl;
    result[seq_left] += (((1.0 / 6.0) * source[seq_right]) * q);
  }

  // Right
  if (this->stretching->hasDirichletBoundaryRight(dim)) {
    result[seq_right] = 0.0;  // source[seq_right];
  } else {
    result[seq_right] = fr;
  }

  index.resetToLeftLevelZero(dim);
}

}  // namespace pde
}  // namespace SGPP
