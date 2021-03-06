// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef LINEARSTRETCHEDTRUNCATEDBOUNDARYGRID_HPP
#define LINEARSTRETCHEDTRUNCATEDBOUNDARYGRID_HPP

#include <sgpp/base/grid/Grid.hpp>

#include <sgpp/globaldef.hpp>


namespace SGPP {
namespace base {

/**
 * grid with linear base functions with boundaries, pentagon cut
 */
class LinearStretchedBoundaryGrid : public Grid {
 protected:
  explicit LinearStretchedBoundaryGrid(std::istream& istr);

 public:
  /**
   * Constructor Linear Truncated Boundary Grid
   *
   * @param dim the dimension of the grid
   */
  explicit LinearStretchedBoundaryGrid(size_t dim);

  /**
   * Constructor Linear Truncated Boundary Grid
   *
   * @param BB the Stretching of the grid
   */
  explicit LinearStretchedBoundaryGrid(Stretching& BB);

  /**
   * Destructor
   */
  ~LinearStretchedBoundaryGrid() override;

  SGPP::base::GridType getType() override;

  const SBasis& getBasis() override;

  GridGenerator* createGridGenerator() override;

  static Grid* unserialize(std::istream& istr);
};

}  // namespace base
}  // namespace SGPP

#endif /* LINEARSTRETCHEDTRUNCATEDBOUNDARYGRID_HPP */
