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

#include <boost/format.hpp>
#include <votca/tools/tokenizer.h>
#include <votca/xtp/vxc_functionals.h>
#include <votca/xtp/vxc_grid.h>
#include <votca/xtp/vxc_potential.h>

namespace votca {
namespace xtp {
template <class Grid>
Vxc_Potential<Grid>::~Vxc_Potential() {
  if (_setXC) {
    xc_func_end(&xfunc);
    if (_use_separate) {
      xc_func_end(&cfunc);
    }
  }
}
template <class Grid>
double Vxc_Potential<Grid>::getExactExchange(const std::string& functional) {

  double exactexchange = 0.0;
  Vxc_Functionals map;
  tools::Tokenizer tok(functional, " ");
  std::vector<std::string> functional_names = tok.ToVector();

  if (functional_names.size() > 2) {
    throw std::runtime_error("Too many functional names");
  } else if (functional_names.size() < 1) {
    throw std::runtime_error("Specify at least one functional");
  }

  for (const std::string& functional_name : functional_names) {

    int func_id = map.getID(functional_name);
    if (func_id < 0) {
      exactexchange = 0.0;
      break;
    }
    xc_func_type func;
    if (xc_func_init(&func, func_id, XC_UNPOLARIZED) != 0) {
      throw std::runtime_error(
          (boost::format("Functional %s not found\n") % functional_name).str());
    }
    if (exactexchange > 0 && func.cam_alpha > 0) {
      throw std::runtime_error(
          "You have specified two functionals with exact exchange");
    }
    exactexchange += func.cam_alpha;
    xc_func_end(&func);
  }

  return exactexchange;
}
template <class Grid>
void Vxc_Potential<Grid>::setXCfunctional(const std::string& functional) {

  Vxc_Functionals map;
  std::vector<std::string> strs;
  tools::Tokenizer tok(functional, " ,\n\t");
  tok.ToVector(strs);
  xfunc_id = 0;
  _use_separate = false;
  cfunc_id = 0;
  if (strs.size() == 1) {
    xfunc_id = map.getID(strs[0]);
  } else if (strs.size() == 2) {
    xfunc_id = map.getID(strs[0]);
    cfunc_id = map.getID(strs[1]);
    _use_separate = true;
  } else {
    throw std::runtime_error(
        "LIBXC. Please specify one combined or an exchange and a correlation "
        "functionals");
  }

  if (xc_func_init(&xfunc, xfunc_id, XC_UNPOLARIZED) != 0) {
    throw std::runtime_error(
        (boost::format("Functional %s not found\n") % strs[0]).str());
  }
  if (xfunc.info->kind != 2 && !_use_separate) {
    throw std::runtime_error(
        "Your functional misses either correlation or exchange, please specify "
        "another functional, separated by whitespace");
  }
  if (_use_separate) {
    if (xc_func_init(&cfunc, cfunc_id, XC_UNPOLARIZED) != 0) {
      throw std::runtime_error(
          (boost::format("Functional %s not found\n") % strs[1]).str());
    }
    if ((xfunc.info->kind + cfunc.info->kind) != 1) {
      throw std::runtime_error(
          "Your functionals are not one exchange and one correlation");
    }
  }
  _setXC = true;
  return;
}
template <class Grid>
typename Vxc_Potential<Grid>::XC_entry Vxc_Potential<Grid>::EvaluateXC(
    double rho, double sigma) const {

  Vxc_Potential<Grid>::XC_entry result;
  switch (xfunc.info->family) {
    case XC_FAMILY_LDA:
      xc_lda_exc_vxc(&xfunc, 1, &rho, &result.f_xc, &result.df_drho);
      break;
    case XC_FAMILY_GGA:
    case XC_FAMILY_HYB_GGA:
      xc_gga_exc_vxc(&xfunc, 1, &rho, &sigma, &result.f_xc, &result.df_drho,
                     &result.df_dsigma);
      break;
  }
  if (_use_separate) {
    typename Vxc_Potential<Grid>::XC_entry temp;
    // via libxc correlation part only
    switch (cfunc.info->family) {
      case XC_FAMILY_LDA:
        xc_lda_exc_vxc(&cfunc, 1, &rho, &temp.f_xc, &temp.df_drho);
        break;
      case XC_FAMILY_GGA:
      case XC_FAMILY_HYB_GGA:
        xc_gga_exc_vxc(&cfunc, 1, &rho, &sigma, &temp.f_xc, &temp.df_drho,
                       &temp.df_dsigma);
        break;
    }

    result.f_xc += temp.f_xc;
    result.df_drho += temp.df_drho;
    result.df_dsigma += temp.df_dsigma;
  }

  return result;
}

template <class Grid>
double Vxc_Potential<Grid>::EvaluateFXC(double rho) const {

  double result;
  switch (xfunc.info->family) {
    case XC_FAMILY_LDA:
      xc_lda_fxc(&xfunc, 1, &rho, &result);
      break;
    case XC_FAMILY_GGA:
    case XC_FAMILY_HYB_GGA:
      throw std::runtime_error("GGA not implemented");
      break;
  }
  if (_use_separate) {
    double temp;
    // via libxc correlation part only
    switch (cfunc.info->family) {
      case XC_FAMILY_LDA:
        xc_lda_fxc(&cfunc, 1, &rho, &temp);
        break;
      case XC_FAMILY_GGA:
      case XC_FAMILY_HYB_GGA:
        throw std::runtime_error("GGA not implemented");
        break;
    }

    result += temp;
  }

  return result;
}

template <class Grid>
Eigen::Tensor<double, 4> Vxc_Potential<Grid>::precalcFXC(
    const Eigen::MatrixXd density_matrix) const {
  Index nthreads = OPENMP::getMaxThreads();

  std::cout << "started precalc of Fxc" << std::endl;

  // std::vector<Eigen::MatrixXcd> fxc_thread = std::vector<Eigen::MatrixXcd>(
  //     nthreads,
  //     Eigen::MatrixXcd::Zero(density_matrix.rows(), density_matrix.cols()));

  // Initialize tensor to save (ijkl)
  Eigen::Tensor<double, 4> result(density_matrix.cols(), density_matrix.cols(),
                                  density_matrix.cols(), density_matrix.cols());
  result.setZero();
  //#pragma omp parallel for schedule(guided)

  // std::cout<<"number of boxes"<<_grid.getBoxesSize()<<std::endl;
  // std::cout<<"test "<<result(1,2,3,4)<<std::endl;
  for (Index i = 0; i < _grid.getBoxesSize(); ++i) {
    std::cout << "started loop i = " << i << std::endl;
    const GridBox& box = _grid[i];
    if (!box.Matrixsize()) {
      continue;
    }
    const Eigen::MatrixXd DMAT_here = box.ReadFromBigMatrix(density_matrix);

    const Eigen::MatrixXd DMAT_symm = DMAT_here + DMAT_here.transpose();
    double cutoff =
        1.e-40 / double(density_matrix.rows()) / double(density_matrix.rows());
    if (DMAT_here.cwiseAbs2().maxCoeff() < cutoff) {
      continue;
    }
    Eigen::MatrixXcd Fxc_here =
        Eigen::MatrixXcd::Zero(DMAT_here.rows(), DMAT_here.cols());
    const std::vector<Eigen::Vector3d>& points = box.getGridPoints();
    const std::vector<double>& weights = box.getGridWeights();

    const std::vector<const AOShell*> significantShells = box.getShells();

    // Find indices of used basis functions for this box
    std::vector<Index> significantFunctionIndices;

    for (Index s = 0; s < box.getShells().size(); s++) {
      for (Index f = 0; f < significantShells.at(s)->getNumFunc(); f++) {
        significantFunctionIndices.push_back(
            significantShells.at(s)->getStartIndex() + f);
      }
    }

    // iterate over gridpoints
    for (Index p = 0; p < box.size(); p++) {
      Eigen::VectorXd ao = box.CalcAOValues(points[p]);
      // std::cout<<"Number of basis functions "<<ao.size()<<std::endl;
      // std::cout<<"Number of basis functions
      // "<<significantFunctionIndices.size()<<std::endl<<std::endl;

      const double rho = 0.5 * (ao.transpose() * DMAT_symm * ao).value();
      const double weight = weights[p];
      if (rho * weight < 1.e-20) {
        continue;  // skip the rest, if density is very small
      }

      double fxc = EvaluateFXC(rho);

      // Loop over significant shells and calculate (ijkl)
      for (int i2 = 0; i2 < significantFunctionIndices.size(); i2++) {
        for (int j = 0; j < significantFunctionIndices.size(); j++) {
          for (int k = 0; k < significantFunctionIndices.size(); k++) {
            for (int l = 0; l < significantFunctionIndices.size(); l++) {
              // std::cout<<"value =
              // "<<weight*ao(i)*ao(j)*fxc*ao(k)*ao(l)<<std::endl;
              result(significantFunctionIndices.at(i2),
                     significantFunctionIndices.at(j),
                     significantFunctionIndices.at(k),
                     significantFunctionIndices.at(l)) +=
                  weight * ao(i2) * ao(j) * fxc * ao(k) * ao(l);
              // std::cout<<"test"<<std::endl;
            }
          }
        }
      }
    }
  }
  std::cout << "Finished precalc" << std::endl;
  return result;
}

template <class Grid>
Eigen::MatrixXcd Vxc_Potential<Grid>::IntegrateFXC(
    const Eigen::MatrixXd& density_matrix,
    const Eigen::MatrixXcd& perturbation) const {

  Index nthreads = OPENMP::getMaxThreads();

  std::vector<Eigen::MatrixXcd> fxc_thread = std::vector<Eigen::MatrixXcd>(
      nthreads,
      Eigen::MatrixXcd::Zero(density_matrix.rows(), density_matrix.cols()));

#pragma omp parallel for schedule(guided)
  for (Index i = 0; i < _grid.getBoxesSize(); ++i) {
    const GridBox& box = _grid[i];
    if (!box.Matrixsize()) {
      continue;
    }
    const Eigen::MatrixXd DMAT_here = box.ReadFromBigMatrix(density_matrix);
    const Eigen::MatrixXcd perturbation_here =
        box.ReadFromBigMatrix(perturbation);
    const Eigen::MatrixXd DMAT_symm = DMAT_here + DMAT_here.transpose();
    double cutoff =
        1.e-40 / double(density_matrix.rows()) / double(density_matrix.rows());
    if (DMAT_here.cwiseAbs2().maxCoeff() < cutoff) {
      continue;
    }
    Eigen::MatrixXcd Fxc_here =
        Eigen::MatrixXcd::Zero(DMAT_here.rows(), DMAT_here.cols());
    const std::vector<Eigen::Vector3d>& points = box.getGridPoints();
    const std::vector<double>& weights = box.getGridWeights();

    // iterate over gridpoints
    for (Index p = 0; p < box.size(); p++) {
      Eigen::VectorXd ao = box.CalcAOValues(points[p]);
      const double rho = 0.5 * (ao.transpose() * DMAT_symm * ao).value();
      const double weight = weights[p];
      if (rho * weight < 1.e-20) {
        continue;  // skip the rest, if density is very small
      }

      double fxc = EvaluateFXC(rho);
      std::complex<double> pert_coeff =
          (ao.transpose() * perturbation_here * ao).value();
      Fxc_here += weight * pert_coeff * fxc * ao * ao.transpose();
    }
    box.AddtoBigMatrix(fxc_thread[OPENMP::getThreadId()], Fxc_here);
  }

  Eigen::MatrixXcd Fxc = std::accumulate(
      fxc_thread.begin(), fxc_thread.end(),
      Eigen::MatrixXcd::Zero(density_matrix.rows(), density_matrix.cols())
          .eval());
  return Fxc;
}

template <class Grid>
Mat_p_Energy Vxc_Potential<Grid>::IntegrateVXC(
    const Eigen::MatrixXd& density_matrix) const {

  Index nthreads = OPENMP::getMaxThreads();

  std::vector<Eigen::MatrixXd> vxc_thread = std::vector<Eigen::MatrixXd>(
      nthreads,
      Eigen::MatrixXd::Zero(density_matrix.rows(), density_matrix.cols()));
  std::vector<double> Exc_thread = std::vector<double>(nthreads, 0.0);

#pragma omp parallel for schedule(guided)
  for (Index i = 0; i < _grid.getBoxesSize(); ++i) {
    const GridBox& box = _grid[i];
    if (!box.Matrixsize()) {
      continue;
    }
    double EXC_box = 0.0;
    const Eigen::MatrixXd DMAT_here = box.ReadFromBigMatrix(density_matrix);
    const Eigen::MatrixXd DMAT_symm = DMAT_here + DMAT_here.transpose();
    double cutoff =
        1.e-40 / double(density_matrix.rows()) / double(density_matrix.rows());
    if (DMAT_here.cwiseAbs2().maxCoeff() < cutoff) {
      continue;
    }
    Eigen::MatrixXd Vxc_here =
        Eigen::MatrixXd::Zero(DMAT_here.rows(), DMAT_here.cols());
    const std::vector<Eigen::Vector3d>& points = box.getGridPoints();
    const std::vector<double>& weights = box.getGridWeights();

    // iterate over gridpoints
    for (Index p = 0; p < box.size(); p++) {
      Eigen::MatrixX3d ao_grad = Eigen::MatrixX3d::Zero(box.Matrixsize(), 3);
      Eigen::VectorXd ao = box.CalcAOValue_and_Grad(ao_grad, points[p]);
      const double rho = 0.5 * (ao.transpose() * DMAT_symm * ao).value();
      const double weight = weights[p];
      if (rho * weight < 1.e-20) {
        continue;  // skip the rest, if density is very small
      }
      const Eigen::Vector3d rho_grad = ao.transpose() * DMAT_symm * ao_grad;
      const double sigma = (rho_grad.transpose() * rho_grad).value();
      const Eigen::VectorXd grad = ao_grad * rho_grad;
      typename Vxc_Potential<Grid>::XC_entry xc = EvaluateXC(rho, sigma);
      EXC_box += weight * rho * xc.f_xc;
      auto addXC = weight * (0.5 * xc.df_drho * ao + 2.0 * xc.df_dsigma * grad);
      Vxc_here.noalias() += addXC * ao.transpose();
    }
    box.AddtoBigMatrix(vxc_thread[OPENMP::getThreadId()], Vxc_here);
    Exc_thread[OPENMP::getThreadId()] += EXC_box;
  }

  double EXC = std::accumulate(Exc_thread.begin(), Exc_thread.end(), 0.0);
  Eigen::MatrixXd Vxc = std::accumulate(
      vxc_thread.begin(), vxc_thread.end(),
      Eigen::MatrixXd::Zero(density_matrix.rows(), density_matrix.cols())
          .eval());

  Mat_p_Energy Oxc(EXC, Vxc + Vxc.transpose());
  return Oxc;
}

template class Vxc_Potential<Vxc_Grid>;

}  // namespace xtp
}  // namespace votca
