/*
 *            Copyright 2009-2019 The VOTCA Development Team
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

#pragma once
#ifndef _VOTCA_XTP_SINV_OPERATOR_H
#define _VOTCA_XTP_SINV_OPERATOR_H

#include <votca/xtp/eigen.h>
#include <votca/xtp/matrixfreeoperator.h>
#include <votca/xtp/threecenter.h>
#include <Eigen/Core>
#include <Eigen/IterativeLinearSolvers>

namespace votca {
namespace xtp {


template <typename MatrixReplacement>
class SINV_OPERATOR : public MatrixFreeOperator {

public:

  SINV_OPERATOR(const MatrixReplacement &A) :
    _Aop(A) {};

  void set_shift(double sigma)
  {
    // sigma is always 0 for us ...
    cg_solver.compute(_Aop);
  }

  //  get a col of the operator
  Eigen::RowVectorXd row(int index) const {
    return this->_Aop.row(index);
  }

  void perform_op(const double *x_in, double *y_out)
  {
    Eigen::Map<const Eigen::VectorXd> x(x_in,_Aop.size());
    Eigen::Map<Eigen::VectorXd> y(y_out,_Aop.size());
    y = cg_solver.solve(x);
    std::cout << "CG #iterations:" << cg_solver.iterations() 
              << " estimated error: " << cg_solver.error() << std::endl; 
  }

  Eigen::Index rows() {return _Aop.rows();}
  Eigen::Index cols() {return _Aop.cols();}

private:

  MatrixReplacement _Aop;
  double _lambda;

  Eigen::ConjugateGradient<MatrixReplacement, 
    Eigen::Lower|Eigen::Upper, Eigen::IdentityPreconditioner> cg_solver;


};

}  // namespace xtp
}  // namespace votca

#endif /* _VOTCA_XTP_SINV_OP_H */
