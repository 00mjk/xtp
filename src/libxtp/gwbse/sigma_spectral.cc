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

#include <votca/xtp/sigma_spectral.h>
#include <votca/xtp/rpa.h>
#include <votca/xtp/threecenter.h>
#include "votca/xtp/vc2index.h"

namespace votca {
  namespace xtp {

    void Sigma_Spectral::PrepareScreening() {
      _EigenSol = _rpa.calculate_eigenvalues();
      return;
    }

    Eigen::VectorXd Sigma_Spectral::CalcCorrelationDiag(const Eigen::VectorXd& frequencies) const {
      const int numeigenvalues = _EigenSol._Omega.size();
      Eigen::VectorXd result = Eigen::VectorXd::Zero(_qptotal);

      if (_HedinApprox) {
        
        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd residues = CalcResidues(s);
          for (int m = 0; m < _qptotal; m++) {
            Eigen::VectorXd rm_x_rm = residues.col(m).cwiseAbs2();
            result(m) += Equation48(rm_x_rm, omega);
          } // Energy level m
        } // Eigenvalues/poles s

      } else {

        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd residues = CalcResidues(s);
          for (int m = 0; m < _qptotal; m++) {
            Eigen::VectorXd rm_x_rm = residues.col(m).cwiseAbs2();
            // TODO: Pass frequency
            result(m) += Equation47(rm_x_rm, omega, _rpa.getRPAInputEnergies()(m + _opt.qpmin - _opt.rpamin));
          } // Energy level m
        } // Eigenvalues/poles s

      }
      
      return result;
    }

    Eigen::MatrixXd Sigma_Spectral::CalcCorrelationOffDiag(const Eigen::VectorXd& frequencies) const {
      const int numeigenvalues = _EigenSol._Omega.size();
      Eigen::MatrixXd result = Eigen::MatrixXd::Zero(_qptotal, _qptotal);
      
      if (_HedinApprox) {

        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd residues = CalcResidues(s);
          for (int m = 0; m < _qptotal; m++) {
            for (int n = m + 1; n < _qptotal; n++) {
              Eigen::VectorXd rm_x_rn = residues.col(m).cwiseProduct(residues.col(n));
              double res = Equation48(rm_x_rn, omega);
              result(m, n) += res;
              result(n, m) += res;
            } // Energy level n
          } // Energy level m
        } // Eigenvalues/poles s

      } else {

        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd residues = CalcResidues(s);
          for (int m = 0; m < _qptotal; m++) {
            for (int n = m + 1; n < _qptotal; n++) {
              Eigen::VectorXd rm_x_rn = residues.col(m).cwiseProduct(residues.col(n));
              // TODO: Pass frequency
              double result_m = Equation47(rm_x_rn, omega, _rpa.getRPAInputEnergies()(m + _opt.qpmin - _opt.rpamin));
              double result_n = Equation47(rm_x_rn, omega, _rpa.getRPAInputEnergies()(n + _opt.qpmin - _opt.rpamin));
              // (m|S(w)|n) = 0.5 * (m|S(e_m)|n) + 0.5 * (m|S(e_n)|n)
              double res = 0.5 * (result_m + result_n);
              result(m, n) += res;
              result(n, m) += res;
            } // Energy level n
          } // Energy level m
        } // Eigenvalues/poles s

      }

      return result;
    }

    Eigen::MatrixXd Sigma_Spectral::CalcResidues(int s) const {
      const int lumo = _opt.homo + 1;
      const int n_occup = lumo - _opt.rpamin;
      const int n_unocc = _opt.rpamax - _opt.homo;
      const int auxsize = _Mmn.auxsize(); // Size of gwbasis
      const Eigen::VectorXd& xpy = _EigenSol._XpY.col(s);
      vc2index vc = vc2index(0, 0, n_unocc);
      // TODO: Use size _rpatotal x _qptotal?
      Eigen::MatrixXd residues = Eigen::MatrixXd::Zero(_rpatotal, _rpatotal);
      
      for (int v = 0; v < n_occup; v++) {
        const Eigen::MatrixXd Mmn_vT =
                _Mmn[v].block(n_occup, 0, n_unocc, auxsize).transpose();
        const Eigen::VectorXd xpyv = xpy.segment(vc.I(v, 0), n_unocc);
        for (int m = 0; m < _rpatotal; m++) {
          const Eigen::MatrixXd fc =
                  _Mmn[m + _opt.qpmin - _opt.rpamin] * Mmn_vT;
          residues.col(m) += fc * xpyv;
        } // MO m
      } // Occupied MO v

      return residues;
    }

    double Sigma_Spectral::Equation47(const Eigen::VectorXd& A12, double omega, double frequency) const {
      const double eta = 1e-6;
      const int lumo = _opt.homo + 1;
      const int n_occup = lumo - _opt.qpmin;
      const int n_unocc = _opt.qpmax - _opt.homo;
      
      Eigen::ArrayXd B12 = -_rpa.getRPAInputEnergies().array() + frequency;
      B12.segment(0, n_occup) += omega;
      B12.segment(n_occup, n_unocc) -= omega;

      const Eigen::ArrayXd numer = A12.array() * B12;
      const Eigen::ArrayXd denom = B12.abs2() + eta * eta;
      
      return numer.cwiseQuotient(denom).sum();
    }

    double Sigma_Spectral::Equation48(const Eigen::VectorXd& A12, double omega) const {
      const int lumo = _opt.homo + 1;
      const int n_occup = lumo - _opt.qpmin;
      
      double s1 = A12.head(n_occup).sum();
      double s2 = A12.sum();

      return 2 * (s1 - s2) / omega;
    }

  }
};
