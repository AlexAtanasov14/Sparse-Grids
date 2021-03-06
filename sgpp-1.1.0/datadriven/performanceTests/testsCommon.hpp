/*
 * test_datadrivenCommon.cpp
 *
 *  Created on: Jul 30, 2015
 *      Author: pfandedd
 */

#pragma once

#include <sgpp/base/datatypes/DataMatrix.hpp>
#include <sgpp/base/operation/BaseOpFactory.hpp>
#include <sgpp/globaldef.hpp>

std::string uncompressFile(std::string fileName);

void doRandomRefinements(SGPP::base::AdpativityConfiguration& adaptConfig,
SGPP::base::Grid& grid, SGPP::base::GridGenerator& gridGen);

void doDirectedRefinements(SGPP::base::AdpativityConfiguration& adaptConfig,
SGPP::base::Grid& grid, SGPP::base::GridGenerator& gridGen);
