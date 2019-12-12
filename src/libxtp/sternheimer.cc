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

#include "votca/xtp/ERIs.h"
#include <fstream>
#include <votca/tools/property.h>
#include <votca/xtp/aobasis.h>
#include <votca/xtp/aomatrix.h>
#include <votca/xtp/aomatrix3d.h>
#include <votca/xtp/logger.h>
#include <votca/xtp/multishift.h>
#include <votca/xtp/orbitals.h>
#include <votca/xtp/padeapprox.h>
#include <votca/xtp/sternheimer.h>

namespace votca {
namespace xtp {

void Sternheimer::Initialize() {

  this->_num_occ_lvls = _orbitals.getNumberOfAlphaElectrons();
  this->_basis_size = _orbitals.getBasisSetSize();
  this->_overlap_Matrix = OverlapMatrix();
  this->_density_Matrix = _orbitals.DensityMatrixGroundState();
  this->_mo_coefficients = _orbitals.MOs().eigenvectors();
  this->_mo_energies = _orbitals.MOs().eigenvalues();
  this->_inverse_overlap = _overlap_Matrix.inverse();
  this->_Hamiltonian_Matrix = Hamiltonian();
}

void Sternheimer::initializeMultishift(int size) {
  _multishift.setMatrixSize(size);
}

Eigen::MatrixXcd Sternheimer::OverlapMatrix() {

  AOBasis basis = _orbitals.SetupDftBasis();
  AOOverlap overlap;
  overlap.Fill(basis);
  return overlap.Matrix().cast<std::complex<double>>();
}

Eigen::MatrixXcd Sternheimer::DensityMatrix() {
  return _orbitals.DensityMatrixGroundState().cast<std::complex<double>>();
}

Eigen::MatrixXcd Sternheimer::Hamiltonian() {

  const Eigen::MatrixXd& mo_coefficients = _orbitals.MOs().eigenvectors();
  return (_overlap_Matrix * mo_coefficients *
          _orbitals.MOs().eigenvalues().asDiagonal() *
          mo_coefficients.transpose() * _overlap_Matrix)
      .cast<std::complex<double>>();
}

Eigen::MatrixXcd Sternheimer::CoulombMatrix() {

  AOBasis basis = _orbitals.SetupDftBasis();
  AOCoulomb coulomb;
  coulomb.Fill(basis);
  return coulomb.Matrix().cast<std::complex<double>>();
}

std::vector<std::complex<double>> Sternheimer::BuildGrid(
    double omega_start, double omega_end, int steps,
    double imaginary_shift) const {

  std::vector<std::complex<double>> grid;

  double stepsize = (omega_end - omega_start) / steps;
  const double ev2hrt = votca::tools::conv::ev2hrt;
  std::complex<double> d(1, 0);
  std::complex<double> i(0, 1);

  for (int n = 0; n <= steps; n++) {
    // Converts from input eV to Hartree
    grid.push_back(omega_start * ev2hrt + n * stepsize * ev2hrt +
                   imaginary_shift * i * ev2hrt);
  }

  return grid;
}

Eigen::MatrixXcd Sternheimer::SternheimerLHS(
    const Eigen::MatrixXcd& hamiltonian,
    const Eigen::MatrixXcd& inverse_overlap, double eps,
    std::complex<double> omega, bool pm) const {
  Eigen::MatrixXcd Identity_cmplx =
      Eigen::MatrixXd::Identity(_basis_size, _basis_size)
          .cast<std::complex<double>>();
  std::complex<double> temp = eps + omega;
  if (pm != true) {
    temp = (eps - omega);
  }
  Eigen::MatrixXcd LHS =
      (inverse_overlap * hamiltonian - temp * Identity_cmplx);
  return LHS;
}

Eigen::VectorXcd Sternheimer::SternheimerRHS(
    const Eigen::MatrixXcd& inverse_overlap, const Eigen::MatrixXcd& density,
    const Eigen::MatrixXcd& pertubation, const Eigen::VectorXd& coeff) const {

  Eigen::VectorXcd RHS = -1 * (inverse_overlap - density) * pertubation * coeff;
  return RHS;
}

std::vector<Eigen::MatrixXcd> Sternheimer::DeltaNOneShot(
    std::vector<std::complex<double>> w,
    const Eigen::MatrixXd& pertubation) const {

  double alpha = 2 * (_mo_energies(_num_occ_lvls) - _mo_energies(2));

  std::vector<Eigen::MatrixXcd> solution_p;
  std::vector<Eigen::MatrixXcd> solution_m;

  Eigen::MatrixXcd H_new;
  Eigen::MatrixXcd LHS_P;
  Eigen::MatrixXcd LHS_M;

  Eigen::VectorXcd RHS;

  Multishift::MultiShiftResult result;

  for (int v = 0; v < _num_occ_lvls; v++) {

    RHS = SternheimerRHS(_inverse_overlap, _density_Matrix, pertubation,
                         _mo_coefficients.col(v));

    for (int i = 0; i < w.size(); i++) {

      if (v == 0) {
        solution_p.push_back(Eigen::MatrixXcd::Zero(_basis_size, _basis_size));
      }

      if (i == 0) {
        LHS_P = SternheimerLHS(_Hamiltonian_Matrix, _inverse_overlap,
                               _mo_energies(v), w[i], true);
        solution_p[i].col(v) = LHS_P.colPivHouseholderQr().solve(RHS);
      } else {
        LHS_P = SternheimerLHS(_Hamiltonian_Matrix, _inverse_overlap,
                               _mo_energies(v), w[i], true);
        solution_p[i].col(v) = LHS_P.colPivHouseholderQr().solve(RHS);
      }

    }
    for (int i = 0; i < w.size(); i++) {

      if (v == 0) {
        solution_m.push_back(Eigen::MatrixXcd::Zero(_basis_size, _basis_size));
      }

      if (i == 0) {
        LHS_M = SternheimerLHS(_Hamiltonian_Matrix, _inverse_overlap,
                               _mo_energies(v), w[i], false);
        solution_m[i].col(v) = LHS_M.colPivHouseholderQr().solve(RHS);
      } else {
        LHS_M = SternheimerLHS(_Hamiltonian_Matrix, _inverse_overlap,
                               _mo_energies(v), w[i], false);

        solution_m[i].col(v) = LHS_M.colPivHouseholderQr().solve(RHS);
      }
    }
  }
  std::vector<Eigen::MatrixXcd> delta_n;
  for (int m = 0; m < w.size(); m++) {

    delta_n.push_back(Eigen::MatrixXcd::Zero(_basis_size, _basis_size));

    delta_n[m] += 2 * _mo_coefficients * solution_p[m].transpose() +
                  2 * _mo_coefficients * solution_m[m].transpose();
  }
  return delta_n;
}

Eigen::MatrixXcd Sternheimer::DeltaNSelfConsistent(
    std::complex<double> w, const Eigen::MatrixXd& initGuess) const {
  Eigen::MatrixXcd solution_p =
      Eigen::MatrixXcd::Zero(_basis_size, _num_occ_lvls);
  Eigen::MatrixXcd solution_m =
      Eigen::MatrixXcd::Zero(_basis_size, _num_occ_lvls);

  double e_field = 1E-1;

  Eigen::MatrixXcd pertubation = -e_field * initGuess;
  Eigen::MatrixXcd delta_n_old = Eigen::MatrixXcd::Zero(_basis_size, _basis_size);
  Eigen::MatrixXcd delta_n_new = Eigen::MatrixXcd::Zero(_basis_size, _basis_size);

  //AOBasis dftbasis = _orbitals.SetupDftBasis();
  //AOBasis auxbasis = _orbitals.SetupAuxBasis();
  //ERIs eris;
  // eris.Initialize(dftbasis, auxbasis);
  // eris.Initialize_4c_small_molecule(dftbasis);
  
  Eigen::MatrixXcd V_ext = (-e_field * initGuess).cast<std::complex<double>>();
  std::vector<Eigen::MatrixXcd> LHS;

  double diff_new = 1000000;
  double diff_old = 1000000;


  for (int n = 0; n < 1; n++) {
    for (int v = 0; v < _num_occ_lvls; v++) {
      Eigen::MatrixXcd RHS =
          SternheimerRHS(_inverse_overlap, _density_Matrix, pertubation,
                         _mo_coefficients.col(v));
          //std::cout<<"RHS_norm"<<RHS.norm()<<std::endl;
      if (real(w) < -1) {
        // Eigen::MatrixXcd H_new =
        //     _Hamiltonian_Matrix +
        //     alpha * _density_Matrix.transpose();
        // Eigen::MatrixXcd LHS_P = SternheimerLHS(
        //     _Hamiltonian_Matrix, _inverse_overlap, _mo_energies(v), w, true);
        // solution_p.col(v) = LHS_P.colPivHouseholderQr().solve(RHS);
      } else {
        Eigen::MatrixXcd LHS_P = SternheimerLHS(
            _Hamiltonian_Matrix, _inverse_overlap, _mo_energies(v), w, true);

        solution_p.col(v) = LHS_P.colPivHouseholderQr().solve(RHS);
        if((LHS_P*solution_p.col(v)-RHS).norm()>1E-12){
          std::cout<<"Solver failed for frequency " << w <<std::endl;
          std::cout<<"res=" <<(LHS_P*solution_p.col(v)-RHS).norm()<<std::endl;
        }
      }

      if (real(w) < -1) {
        // Eigen::MatrixXcd H_new =
        //     _Hamiltonian_Matrix +
        //     alpha  * _density_Matrix.transpose();
        // Eigen::MatrixXcd LHS_M = SternheimerLHS(
        //     _Hamiltonian_Matrix, _inverse_overlap, _mo_energies(v), w, false);
        // solution_m.col(v) = LHS_M.
        // colPivHouseholderQr().solve(RHS);
      } else {
        Eigen::MatrixXcd LHS_M = SternheimerLHS(
             _Hamiltonian_Matrix, _inverse_overlap, _mo_energies(v), w, false);
        solution_m.col(v) = LHS_M.colPivHouseholderQr().solve(RHS);
        if((LHS_M*solution_m.col(v)-RHS).norm()>1E-12){
          std::cout<<"Solver failed for frequency " << w <<std::endl;
          std::cout<<"res=" <<(LHS_M*solution_m.col(v)-RHS).norm()<<std::endl;
        }
      }
    }

    delta_n_old = delta_n_new;
    delta_n_new = 2 * _mo_coefficients.block(0, 0, _basis_size, _num_occ_lvls) *
                      solution_p.transpose() +
                  2 * _mo_coefficients.block(0, 0, _basis_size, _num_occ_lvls) *
                      solution_m.transpose();

    //Eigen::MatrixXcd contract = eris.ContractRightIndecesWithMatrix(delta_n_new);
    // for(int i=0; i<contract.rows();i++){
    //   for (int j=0; j<contract.cols(); j++){
        
    //       std::cout<<"sym diff ="<<delta_n_new(i,j)-delta_n_new(j,i)<<" iteration "<<n<<std::endl;
        
    //   }
    // }
    
    //  std::cout << "Delta_N at step  " << n << "\n"
    //            << (_overlap_Matrix * delta_n_new).trace() << std::endl;
    //            eris.Initialize_4c_small_molecule(dftbasis);
    //Eigen::MatrixXcd test2 = eris.FourCenterTest(delta_n_new);
    // std::cout<<"test diff 4c = "<<(contract-test2.matrix()).norm()<<std::endl;
    // std::cout<<"4c norm = "<<(test2.matrix()).norm()<<std::endl;
    // std::cout<<"contract norm = "<<(contract).norm()<<std::endl;
    //pertubation = V_ext + test2.matrix();
    //std::cout << "Contract norm = " <<std::endl<< eris.ContractRightIndecesWithMatrix(delta_n_new).norm() << std::endl;
    
    //Mat_p_Energy test = eris.CalculateERIs_4c_small_molecule(_density_Matrix.real());
    
    //std::cout<<"diff="<< (test.matrix()-test2).norm()<<std::endl;
    //std::cout << "Four Center Test = " << std::endl<<test.norm() <<std::endl;
    //std::cout << "Difference Norm " << std::endl<<(contract-test).norm() <<std::endl;
    //std::cout << "Dn = " <<std::endl<< delta_n_new << std::endl;
    //throw std::exception();

    // diff_old=diff_new;
    // diff_new=(delta_n_new - delta_n_old).squaredNorm();
    // std::cout<<"diff="<<diff_new<<std::endl;
    // if ((delta_n_new - delta_n_old).squaredNorm() < 1e-9) {
    //   std::cout<<"converged after "<<n<<" iterations"<<std::endl;
    //   return delta_n_new;
    // }
    // if(diff_new>diff_old){
    //   std::cout<<"stopped after "<<n<<" iterations"<<std::endl;
    //   return delta_n_old;
    // }
  }
  // std::cout << "Not Converged, diff = "
  //           << (delta_n_new - delta_n_old).squaredNorm() << "w=" << w
  //           << std::endl;
  return delta_n_new;
}

std::vector<Eigen::Matrix3cd> Sternheimer::Polarisability(
    double omega_start, double omega_end, int steps, double imaginary_shift,
    double lorentzian_broadening, int resolution_output) const {

  std::vector<std::complex<double>> grid_w =
      BuildGrid(omega_start, omega_end, steps, imaginary_shift);

  for(int i=0;i<grid_w.size();i++){
     std::cout<<grid_w[i]<<std::endl;
  }

  std::vector<std::complex<double>> w = BuildGrid(
      omega_start, omega_end, resolution_output, lorentzian_broadening);

  std::vector<Eigen::Matrix3cd> Polar;
  std::vector<Eigen::Matrix3cd> Polar_pade;

  for(int i=0; i<grid_w.size(); i++){
    Polar.push_back(Eigen::Matrix3cd::Zero());
  }

  PadeApprox pade_1;
  // PadeApprox pade_2;
  // PadeApprox pade_3;
  PadeApprox pade_4;
  // PadeApprox pade_5;
  PadeApprox pade_6;
  pade_1.initialize(4 * grid_w.size());
  // pade_2.initialize(2*grid_w.size());
  // pade_3.initialize(2*grid_w.size());
  pade_4.initialize(4 * grid_w.size());
  // pade_5.initialize(2*grid_w.size());
  pade_6.initialize(4 * grid_w.size());

  

  AOBasis basis = _orbitals.SetupDftBasis();
  AODipole dipole;
  dipole.Fill(basis);

#pragma omp parallel for
  for (int n = 0; n < grid_w.size(); n++) {
    for (int i = 0; i < 3; i++) {
      Eigen::MatrixXcd delta_n =
          DeltaNSelfConsistent(grid_w[n], dipole.Matrix()[i]);
      for (int j = i; j < 3; j++) {
        Polar[n](i, j) = -(delta_n.cwiseProduct(dipole.Matrix()[j])).sum();
      }
    }
    for (int i = 2; i < 3; i++) {
      for (int j = i + 1; j < 3; j++) {
        Polar[n](j, i) = conj(Polar[n](i, j));
      }
    }
    
    std::cout << "Done with w=" << grid_w[n] << std::endl;

  }
  // pade_4.printInfo();
  
  for(int n=0;n<Polar.size(); n++){
    pade_1.addPoint(grid_w[n], Polar[n](0, 0));
    pade_1.addPoint(conj(grid_w[n]), conj(Polar[n](0, 0)));
    pade_1.addPoint(-grid_w[n], conj(Polar[n](0, 0)));
    pade_1.addPoint(-conj(grid_w[n]), Polar[n](0, 0));

    // pade_1.printInfo();

    // throw std::exception();

    // pade_2.addPoint(grid_w[n], Polar(0,1));
    // pade_2.addPoint(conj(grid_w[n]), conj(Polar(0,1)));

    // pade_3.addPoint(grid_w[n], Polar(0,2));
    // pade_3.addPoint(conj(grid_w[n]), conj(Polar(0,2)));

    pade_4.addPoint(grid_w[n], Polar[n](1, 1));
    pade_4.addPoint(conj(grid_w[n]), conj(Polar[n](1, 1)));
    pade_4.addPoint(-grid_w[n], conj(Polar[n](1, 1)));
    pade_4.addPoint(-conj(grid_w[n]), Polar[n](1, 1));

    // pade_5.addPoint(grid_w[n], Polar(1,2));
    // pade_5.addPoint(conj(grid_w[n]), conj(Polar(1,2)));

    pade_6.addPoint(grid_w[n], Polar[n](2, 2));
    pade_6.addPoint(conj(grid_w[n]), conj(Polar[n](2, 2)));
    pade_6.addPoint(-grid_w[n], conj(Polar[n](2, 2)));
    pade_6.addPoint(-conj(grid_w[n]), Polar[n](2, 2));

  }

  for (std::complex<double> w : w) {
    Polar_pade.push_back(Eigen::Matrix3cd::Zero());
    Polar_pade[Polar_pade.size() - 1](0, 0) = pade_1.evaluatePoint(w);
    // Polar_pade[Polar_pade.size()-1](0,1)=pade_2.evaluatePoint(w);
    // Polar_pade[Polar_pade.size()-1](0,2)=pade_3.evaluatePoint(w);
    // Polar_pade[Polar_pade.size()-1](1,0)=Polar_pade[Polar_pade.size()-1](0,1);
    Polar_pade[Polar_pade.size() - 1](1, 1) = pade_4.evaluatePoint(w);
    // Polar_pade[Polar_pade.size()-1](1,2)=pade_5.evaluatePoint(w);
    // Polar_pade[Polar_pade.size()-1](2,0)=Polar_pade[Polar_pade.size()-1](0,2);
    // Polar_pade[Polar_pade.size()-1](2,1)=Polar_pade[Polar_pade.size()-1](1,2);
    Polar_pade[Polar_pade.size() - 1](2, 2) = pade_6.evaluatePoint(w);
    // pade_1.printAB();
  }
  printIsotropicAverage(Polar_pade, w);
  return Polar_pade;
}
void Sternheimer::printIsotropicAverage(
    std::vector<Eigen::Matrix3cd>& polar,
    std::vector<std::complex<double>>& grid) const {
  for (int i = 0; i < polar.size(); i++) {
    std::cout << real(grid.at(i)) * votca::tools::conv::hrt2ev << " "
              << real((polar.at(i)(2, 2))) + real(polar.at(i)(1, 1)) +
                     real(polar.at(i)(0, 0)) / 3
              << std::endl;
  }
}
}  // namespace xtp
}  // namespace votca