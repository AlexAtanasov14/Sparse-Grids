// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

#ifndef PDESOLVER_HPP
#define PDESOLVER_HPP

#include <sgpp/base/grid/Grid.hpp>
#include <sgpp/base/grid/common/BoundingBox.hpp>
#include <sgpp/base/grid/common/Stretching.hpp>
#include <sgpp/base/datatypes/DataVector.hpp>
#include <sgpp/base/tools/GridPrinter.hpp>

#include <sgpp/globaldef.hpp>

#include <vector>
#include <string>

namespace SGPP {
namespace pde {

/**
 * This class provides defines a implements basic task and tools which are
 * useful while solving PDEs. E.g. grid construction, simple grid evaluation tools
 * grid printing, initial grid refinement etc.
 *
 */
class PDESolver {
 protected:
  /// the number of levels used for an regular grid
  int levels;
  /// the dimension of the grid
  size_t dim;
  /// stores if the grid was created inside the solver
  bool bGridConstructed;
  /// Stores Pointer to the SGPP::base::Grid's Bounding Box
  SGPP::base::BoundingBox* myBoundingBox;
  /// Stores Pointer to the Girs's Storage
  SGPP::base::GridStorage* myGridStorage;
  /// The Sparse SGPP::base::Grid needed in this classificator
  SGPP::base::Grid* myGrid;
  /**
   * This function calculates for every grid point the value
   * of a normal distribution given by norm_mu and norm_sigma.
   * The result is stored dehierarchized in alpha.
   *
   * @param alpha contains dehierarchized sparse grid coefficients containing the values of the
   * multi dimensional normal distribution after call
   * @param norm_mu the expected values of the normal distribution for every grid dimension
   * @param norm_sigma the standard deviation of the normal distribution for every grid dimension
   */
  virtual void getGridNormalDistribution(SGPP::base::DataVector& alpha,
                                         std::vector<float_t>& norm_mu,
                                         std::vector<float_t>& norm_sigma);

 public:
  /**
   * Std-Constructor of the solver
   */
  PDESolver();

  /**
   * Std-Destructor of the solver
   */
  virtual ~PDESolver();

  /**
   * Use this routine the construct a regular grid to solve a PDE
   *
   * @param myBoundingBox reference to a bounding box that describes the grid
   * @param level number of the regular's grid levels
   */
  virtual void constructGrid(SGPP::base::BoundingBox& myBoundingBox, int level) = 0;

  /**
   * Sets the grid used in this BlackScholes Solver by an given serialized string
   * of the grid.
   *
   * @param serializedGrid a string that describes the grid that should be used in this solver
   */
  void setGrid(const std::string& serializedGrid);

  /**
   * gets the a string the describes the grid which is currently used to solve
   *
   * @return a string containing a serialized grid
   */
  std::string getGrid() const;

  /**
   * deletes the grid created within that solver
   */
  void deleteGrid();

  /**
   * Refines a grid by taking the grid's coefficients into account. This refinement method
   * refines the grid based on the surplus by refining grid points with big surpluses
   * first. The number of grid points to refine may be specified by the numRefinePoints parameter.
   *
   * @param alpha a SGPP::base::DataVector containing the grids coefficients
   * @param numRefinePoints the number of grid points that should be refined; if this smaller than
   * zero -> all refineable points will be refined
   * @param dThreshold Threshold for a point's surplus for refining this point
   */
  void refineInitialGridSurplus(SGPP::base::DataVector& alpha, int numRefinePoints,
                                float_t dThreshold);

  /**
   * Refines a grid by taking the grid's coefficients into account. This refinement method
   * refines the grid based on the surplus by refining grid points with big surpluses
   * first.
   * The grid is refined to max. Level!
   *
   * @param alpha a SGPP::base::DataVector containing the grids coefficients
   * @param dThreshold Threshold for a point's surplus for refining this point
   * @param maxLevel maxLevel of refinement
   */
  void refineInitialGridSurplusToMaxLevel(SGPP::base::DataVector& alpha, float_t dThreshold,
                                          SGPP::base::GridStorage::index_type::level_type maxLevel);

  /**
   * Refines a grid by taking the grid's coefficients into account. This refinement method
   * refines the grid based on the surplus by refining grid points with big surpluses
   * first. The number of grid points to refine may be specified by the numRefinePoints parameter.
   *
   * This functions refines the grid only in subdomain specified by a multi-dimensional
   * normal distribution. The normal distribution is given by the parameters norm_mu
   * and norm_sigma which are d-dimensional vectors.
   *
   * @param alpha a SGPP::base::DataVector containing the grids coefficients
   * @param numRefinePoints the number of grid points that should be refined; if this smaller than
   * zero -> all refineable points will be refined
   * @param dThreshold Threshold for a point's surplus for refining this point
   * @param norm_mu the expected values of the normal distribution for every grid dimension
   * @param norm_sigma the standard deviation of the normal distribution for every grid dimension
   */
  void refineInitialGridSurplusSubDomain(SGPP::base::DataVector& alpha, int numRefinePoints,
                                         float_t dThreshold, std::vector<float_t>& norm_mu,
                                         std::vector<float_t>& norm_sigma);

  /**
   * Refines a grid by taking the grid's coefficients into account. This refinement method
   * refines the grid based on the surplus by refining grid points with big surpluses
   * first.
   * The grid is refined to max. Level!
   *
   * This functions refines the grid only in subdomain specified by a multi-dimensional
   * normal distribution. The normal distribution is given by the parameters norm_mu
   * and norm_sigma which are d-dimensional vectors.
   *
   * @param alpha a SGPP::base::DataVector containing the grids coefficients
   * @param dThreshold Threshold for a point's surplus for refining this point
   * @param maxLevel maxLevel of refinement
   * @param norm_mu the expected values of the normal distribution for every grid dimension
   * @param norm_sigma the standard deviation of the normal distribution for every grid dimension
   */
  void refineInitialGridSurplusToMaxLevelSubDomain(
      SGPP::base::DataVector& alpha, float_t dThreshold,
      SGPP::base::GridStorage::index_type::level_type maxLevel, std::vector<float_t>& norm_mu,
      std::vector<float_t>& norm_sigma);

  /**
   * Coarsens a grid by taking the grid's coefficients into account. This coarsen method
   * coarsens the grid based on the surplus by coarsening grid points with small surpluses
   * first.
   *
   * @param alpha a SGPP::base::DataVector containing the grids coefficients
   * @param dThreshold Threshold for a point's surplus for coarsening this point
   */
  void coarsenInitialGridSurplus(SGPP::base::DataVector& alpha, float_t dThreshold);

  /**
   * Determines the value of the function in the d-dimensional space
   *
   * @param evalPoint coordinates of the point at which the function should be evaluated
   * @param alpha the ansatzfunctions' coefficients
   *
   * @return price of option for given point
   */
  float_t evaluatePoint(std::vector<float_t>& evalPoint, SGPP::base::DataVector& alpha);

  /**
   * Evaluates the sparse grid's function given by the stored grid and the alpha coefficients.
   * on different points specified in EvaluationPoints and stores the result into FunctionValues.
   *
   * @param alpha the sparse grid's coefficients
   * @param FunctionValues SGPP::base::DataVector into the which the result of function's evaluation
   * is stored
   * @param EvaluationPoints SGPP::base::DataMatrix that contains the points at which the sparse
   * grid's function is evaluated
   */
  void evaluateCuboid(SGPP::base::DataVector& alpha, SGPP::base::DataVector& FunctionValues,
                      SGPP::base::DataMatrix& EvaluationPoints);

  /**
   * Prints the level,index pairs of the grid for each Gridpoint to a file.
   *
   * @param tfilename relative path to file into which the grid's evaluation is written
   */
  virtual void printLevelIndexGrid(std::string tfilename) const;

  /**
   * This is some kind of debug functionality. It writes a file,
   * that can be used with gnuplot the print the grid.
   *
   * Is only implemented for 1D and 2D grids!
   *
   * @param alpha the coefficients of the Sparse Gird's basis functions
   * @param PointesPerDimension the distance between evaluation points
   * @param tfilename absolute path to file into which the grid's evaluation is written
   */
  virtual void printGrid(SGPP::base::DataVector& alpha, float_t PointesPerDimension,
                         std::string tfilename) const;

  /**
   * This is some kind of debug functionality. It writes a file,
   * that can be used with gnuplot the print the grid.
   *
   * Is only implemented for 2D grids!
   *
   * @param alpha the coefficients of the Sparse Gird's basis functions
   * @param PointesPerDimension the distance between evaluation points
   * @param GridArea the area in which the function should be plotted
   * @param tfilename absolute path to file into which the grid's evaluation is written
   */
  virtual void printGridDomain(SGPP::base::DataVector& alpha, float_t PointesPerDimension,
                               SGPP::base::BoundingBox& GridArea, std::string tfilename) const;

  /**
   * Prints the SGPP::base::Grid Points of the Sparse SGPP::base::Grid either with their node basis
   * value
   * or their hierarchical surplus
   *
   * This function is available for all dimensions
   *
   * @param alpha the coefficients of the grid's ansatzfunctions
   * @param tfilename absoulte path to the file the grid is written into
   * @param bSurplus specifies whether the surplus (true) or the node basis value (false) is written
   */
  virtual void printSparseGrid(SGPP::base::DataVector& alpha, std::string tfilename,
                               bool bSurplus) const;

  /**
   * Prints the SGPP::base::Grid Points of the Sparse SGPP::base::Grid either with their node basis
   * value
   * or their hierarchical surplus
   *
   * This function is available for all dimensions.
   *
   * The coordinates of the grid points are pushed the exp function. So
   * log transformed grids can be plotted in cartesion coordinates.
   *
   * @param alpha the coefficients of the grid's ansatzfunctions
   * @param tfilename absoulte path to the file the grid is written into
   * @param bSurplus specifies whether the surplus (true) or the node basis value (false) is written
   */
  virtual void printSparseGridExpTransform(SGPP::base::DataVector& alpha, std::string tfilename,
                                           bool bSurplus) const;

  /**
   * use this to determine the number of grid points, used to solve
   * the current problem
   *
   * @return the number of grid points
   */
  size_t getNumberGridPoints() const;

  /**
   * use this to determine the number of inner grid points, used to solve
   * the current problem
   *
   * @return the number of inner grid points
   */
  size_t getNumberInnerGridPoints() const;

  /**
   * use this the determine the number of dimensions that are currently used
   * in the solver.
   *
   * @return returns the number of the grid's dimensions, if the grid isn't constructed, yet it
   * returns 0
   */
  size_t getNumberDimensions() const;
};
}  // namespace pde
}  // namespace SGPP

#endif /* PDESOLVER_HPP */
