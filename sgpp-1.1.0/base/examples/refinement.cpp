// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

// All SG++ headers
// #include <sgpp_base.hpp>

// Or, better!, include only those that are required
#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/base/grid/Grid.hpp>
#include <sgpp/base/grid/GridStorage.hpp>
#include <sgpp/base/grid/generation/GridGenerator.hpp>
#include <sgpp/base/operation/hash/OperationEval.hpp>
#include <sgpp/base/operation/BaseOpFactory.hpp>
#include <sgpp/base/grid/generation/functors/SurplusRefinementFunctor.hpp>

#include <iostream>

using SGPP::base::DataVector;
using SGPP::base::Grid;
using SGPP::base::GridGenerator;
using SGPP::base::GridIndex;
using SGPP::base::GridStorage;
using SGPP::base::SurplusRefinementFunctor;

// function to interpolate
SGPP::float_t f(SGPP::float_t x0, SGPP::float_t x1) {
  return 16.0 * (x0 - 1) * x0 * (x1 - 1) * x1;
}

int main() {
  // create a two-dimensional piecewise bilinear grid
  size_t dim = 2;
  Grid* grid = Grid::createLinearGrid(dim);
  GridStorage* gridStorage = grid->getStorage();
  std::cout << "dimensionality:                   " << gridStorage->dim() << std::endl;

  // create regular grid, level 3
  size_t level = 3;
  GridGenerator* gridGen = grid->createGridGenerator();
  gridGen->regular(level);
  std::cout << "number of initial grid points:    " << gridStorage->size() << std::endl;

  // create coefficient vector
  DataVector alpha(gridStorage->size());
  alpha.setAll(0.0);
  std::cout << "length of alpha vector:           " << alpha.getSize() << std::endl;

  GridIndex* gp;

  // refine adaptively 5 times
  for (int step = 0; step < 5; step++) {
    // set function values in alpha
    for (size_t i = 0; i < gridStorage->size(); i++) {
      gp = gridStorage->get(i);
      alpha[i] = f(gp->getCoord(0), gp->getCoord(1));
    }

    // hierarchize
    SGPP::op_factory::createOperationHierarchisation(*grid)->doHierarchisation(
      alpha);

    // refine a single grid point each time
    gridGen->refine(new SurplusRefinementFunctor(&alpha, 1));
    std::cout << "refinement step " << step + 1 << ", new grid size: " << alpha.getSize()
         << std::endl;

    // extend alpha vector (new entries uninitialized)
    alpha.resize(gridStorage->size());
  }

  delete grid;
}
