/*
 * Copyright 2009-2018 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE test_hdf5
#include <boost/test/unit_test.hpp>
#include <votca/xtp/checkpoint.h>

BOOST_AUTO_TEST_SUITE(test_hdf5)

BOOST_AUTO_TEST_CASE(checkpoint_file_test) {
    votca::xtp::CheckpointFile cpf("xtp_testing.hdf5");

    // Write orbitals
    votca::xtp::Orbitals orbWrite;

    orbWrite.setBasisSetSize(17);
    orbWrite.setNumberOfLevels(4,13);

    Eigen::VectorXd moeTest = Eigen::VectorXd::Zero(17);
    Eigen::MatrixXd mocTest = Eigen::MatrixXd::Zero(17,17);

    orbWrite.MOEnergies()=moeTest;
    orbWrite.MOCoefficients()=mocTest;

    cpf.WriteOrbitals(orbWrite, "Test Orb");

    // Read Orbitals
    votca::xtp::Orbitals orbRead;


}

BOOST_AUTO_TEST_SUITE_END()
