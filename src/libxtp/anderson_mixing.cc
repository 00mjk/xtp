/*
 *            Copyright 2009-2020 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <boost/format.hpp>
#include <votca/xtp/anderson_mixing.h>

namespace votca {
namespace xtp {

void ANDERSON::UpdateOutput(const Eigen::VectorXd &newOutput) {

  // Check if max mixing history is reached and adding new step to history
  if (_output.size() > _max_history - 1) {
    _output.erase(_output.begin());
  }
  //std::cout << "_output size " << _output.size() << std::endl;  
  _output.push_back(newOutput);
}

void ANDERSON::UpdateInput(const Eigen::VectorXd &newInput) {

  if (_input.size() > _max_history - 1) {
    _input.erase(_input.begin());
  }
    //std::cout << "_input size " << _input.size() << std::endl;  
  _input.push_back(newInput);
}

Eigen::VectorXd ANDERSON::NPAndersonMixing(const double alpha) {

  _iteration++;


std::cout << "_output size " << _output.size() << std::endl;  
std::cout << "_input size " << _input.size() << std::endl;  

  Eigen::VectorXd OutMixed = _output.back();
  Eigen::VectorXd InMixed = _input.back();
  if (_iteration > 1) {

    Eigen::VectorXd DeltaN = _output.back() - _input.back();

    for (Index i_qp = 0; i_qp < _output.back().size(); i_qp++) {

      // Building Linear System for Coefficients FOR EACH ENTRY
      Eigen::MatrixXd A =
          Eigen::MatrixXd::Zero(_input.size() - 1, _input.size() - 1);
      Eigen::VectorXd c = Eigen::VectorXd::Zero(_input.size() - 1);

      for (Index m = 1; m < _input.size(); m++) {

        c(m - 1) = (DeltaN(i_qp) - _output.at(_output.size() - 1 - m)(i_qp) +
                    _input.at(_input.size() - 1 - m)(i_qp)) *
                  DeltaN(i_qp);
        /*c(m - 1) = (DeltaN - _output.at(_output.size() - 1 - m) +
                    _input.at(_input.size() - 1 - m)).cwiseProduct(
                   DeltaN).sum();*/

        for (Index j = 1; j < _input.size(); j++) {

            std::cout << "m,j,element" << m << "  " << j << " " << _output.size() - 1 - m << std::endl;

          A(m - 1, j - 1) =
             (DeltaN(i_qp) - _output.at(_output.size() - 1 - m)(i_qp) +
             _input.at(_input.size() - 1 - m)(i_qp)) *
              (DeltaN(i_qp) - _output.at(_output.size() - 1 - j)(i_qp) +
               _input.at(_input.size() - 1 - j)(i_qp));
        /*  A(m - 1, j - 1) =
              (DeltaN - _output.at(_output.size() - 1 - m) +
               _input.at(_input.size() - 1 - m)).cwiseProduct(
              (DeltaN - _output.at(_output.size() - 1 - j) +
               _input.at(_input.size() - 1 - j))).sum();*/
        }
      }
      // Solving the System to obtain coefficients
      Eigen::VectorXd coefficients = A.fullPivHouseholderQr().solve(c);

      // Mixing the Potentials
      for (Index n = 1; n < _input.size(); n++) {

        OutMixed(i_qp) +=
            coefficients(n - 1) * (_output.at(_output.size() - 1 - n)(i_qp) -
                                   _output.at(_output.size() - 1)(i_qp));
        InMixed(i_qp) +=
            coefficients(n - 1) * (_input.at(_input.size() - 1 - n)(i_qp) -
                                   _input.at(_input.size() - 1)(i_qp));
/*        OutMixed +=
            coefficients(n - 1) * (_output.at(_output.size() - 1 - n) -
                                   _output.at(_output.size() - 1));
        InMixed +=
            coefficients(n - 1) * (_input.at(_input.size() - 1 - n) -
                                   _input.at(_input.size() - 1));*/
      }
    }
  }

  // Returning the linear Mix of Input and Output
  return alpha * OutMixed + (1 - alpha) * InMixed;
}  // namespace xtp
}  // namespace xtp
}  // namespace votca