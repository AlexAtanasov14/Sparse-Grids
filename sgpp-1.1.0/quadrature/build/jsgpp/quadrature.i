// Copyright (C) 2008-today The SG++ project
// This file is part of the SG++ project. For conditions of distribution and
// use, please see the copyright notice provided with SG++ or at
// sgpp.sparsegrids.org

// The Good, i.e. without any modifications
%include "quadrature/src/sgpp/quadrature/Random.hpp"
%include "quadrature/src/sgpp/quadrature/sampling/SampleGenerator.hpp"
%include "quadrature/src/sgpp/quadrature/sampling/NaiveSampleGenerator.hpp"
%include "quadrature/src/sgpp/quadrature/sampling/LatinHypercubeSampleGenerator.hpp"
%include "quadrature/src/sgpp/quadrature/sampling/StratifiedSampleGenerator.hpp"
%include "quadrature/src/sgpp/quadrature/sampling/HaltonSampleGenerator.hpp"

//%apply (int DIM1, long long int* IN_ARRAY1) {(size_t dimensions, long long int* strataPerDimension)};

//%apply std::string *INPUT { std::string& istr };

%apply unsigned int *OUTPUT { unsigned int& l, unsigned int& i };

//%apply std::vector<std::pair<size_t, float_t> > *OUTPUT { std::vector<std::pair<size_t, float_t> >& result };
//%apply std::vector<float_t> *INPUT { std::vector<float_t>& point };
